import queue
import time
import json
import pytest
from unittest.mock import MagicMock, patch
from sample_processor import SampleProcessor


@pytest.fixture
def basic_config():
    return {}


@pytest.fixture
def file_config(tmp_path):
    save_file = str(tmp_path / "output.jsonl")
    return {'save_to_file': save_file}


@pytest.fixture
def processor_basic(basic_config):
    return SampleProcessor(basic_config)


@pytest.fixture
def processor_with_file(file_config):
    return SampleProcessor(file_config)


# --- Initialization ---

class TestInit:
    def test_default_max_len(self, basic_config):
        p = SampleProcessor(basic_config)
        assert p.max_len == 100

    def test_custom_max_len(self):
        p = SampleProcessor({'max_len': 50})
        assert p.max_len == 50

    def test_queue_created_with_max_len(self):
        p = SampleProcessor({'max_len': 10})
        assert p.msg_queue.maxsize == 10

    def test_save_file_created_empty(self, file_config):
        p = SampleProcessor(file_config)
        with open(file_config['save_to_file']) as f:
            assert f.read() == ''

    def test_grapher_none_without_graph_config(self, processor_basic):
        assert processor_basic.grapher is None

    def test_graph_config_imports_plotter(self):
        mock_plotter_instance = MagicMock()
        mock_plotter_class = MagicMock(return_value=mock_plotter_instance)
        with patch.dict('sys.modules', {'accel_plotter': MagicMock(AccelPlotter=mock_plotter_class)}):
            p = SampleProcessor({'graph': {'window_seconds': 5}})
            mock_plotter_class.assert_called_once_with(window_seconds=5)
            assert p.grapher is mock_plotter_instance


# --- on_sample ---

class TestOnSample:
    def test_puts_item_in_queue(self, processor_basic):
        processor_basic.on_sample('{"x": 1}')
        assert not processor_basic.msg_queue.empty()

    def test_queue_item_is_tuple_with_timestamp(self, processor_basic):
        before = time.time()
        processor_basic.on_sample('{"x": 1}')
        after = time.time()
        ts, data = processor_basic.msg_queue.get_nowait()
        assert before <= ts <= after
        assert data == '{"x": 1}'

    def test_queue_respects_maxsize(self):
        p = SampleProcessor({'max_len': 2})
        p.msg_queue.put((time.time(), '{}'))
        p.msg_queue.put((time.time(), '{}'))
        with pytest.raises(queue.Full):
            p.msg_queue.put_nowait((time.time(), '{}'))


# --- sink_file ---

class TestSinkFile:
    def test_writes_json_line(self, processor_with_file, file_config):
        sample = {'x': 1, 'timestamp': 1234567890.0}
        processor_with_file.sink_file(sample)
        with open(file_config['save_to_file']) as f:
            line = f.readline()
        assert json.loads(line) == sample

    def test_appends_multiple_samples(self, processor_with_file, file_config):
        for i in range(3):
            processor_with_file.sink_file({'i': i})
        with open(file_config['save_to_file']) as f:
            lines = f.readlines()
        assert len(lines) == 3

    def test_handles_write_error_gracefully(self, processor_with_file, capsys):
        processor_with_file.save_file = '/nonexistent/path/file.jsonl'
        processor_with_file.sink_file({'x': 1})  # Should not raise
        captured = capsys.readouterr()
        assert 'Error writing to file' in captured.out


# --- sink ---

class TestSink:
    def test_sink_calls_sink_file_when_configured(self, processor_with_file):
        processor_with_file.sink_file = MagicMock()
        sample = {'x': 1}
        processor_with_file.sink(sample)
        processor_with_file.sink_file.assert_called_once_with(sample)

    def test_sink_does_not_call_sink_file_without_config(self, processor_basic):
        processor_basic.sink_file = MagicMock()
        processor_basic.sink({'x': 1})
        processor_basic.sink_file.assert_not_called()

    def test_sink_calls_sink_graph_when_configured(self):
        mock_grapher = MagicMock()
        with patch.dict('sys.modules', {'accel_plotter': MagicMock(AccelPlotter=MagicMock(return_value=mock_grapher))}):
            p = SampleProcessor({'graph': {'window_seconds': 5}})
            p.sink_graph = MagicMock()
            p.sink({'x': 1})
            p.sink_graph.assert_called_once_with({'x': 1})


# --- sink_graph ---

class TestSinkGraph:
    def test_calls_grapher_add_sample(self):
        mock_grapher = MagicMock()
        with patch.dict('sys.modules', {'accel_plotter': MagicMock(AccelPlotter=MagicMock(return_value=mock_grapher))}):
            p = SampleProcessor({'graph': {'window_seconds': 5}})
            sample = {'x': 1}
            p.sink_graph(sample)
            mock_grapher.add_sample.assert_called_once_with(sample)

    def test_handles_graph_error_gracefully(self, capsys):
        mock_grapher = MagicMock()
        mock_grapher.add_sample.side_effect = RuntimeError("graph error")
        with patch.dict('sys.modules', {'accel_plotter': MagicMock(AccelPlotter=MagicMock(return_value=mock_grapher))}):
            p = SampleProcessor({'graph': {'window_seconds': 5}})
            p.sink_graph({'x': 1})  # Should not raise
        captured = capsys.readouterr()
        assert 'Error adding sample to graph' in captured.out


# --- process (integration via start) ---

class TestProcess:
    def test_process_parses_json_and_sinks(self, processor_with_file, file_config):
        processor_with_file.start()
        processor_with_file.on_sample('{"value": 42}')
        time.sleep(0.1)
        with open(file_config['save_to_file']) as f:
            lines = f.readlines()
        assert len(lines) == 1
        data = json.loads(lines[0])
        assert data['value'] == 42
        assert 'timestamp' in data

    def test_process_handles_invalid_json_gracefully(self, processor_basic, capsys):
        processor_basic.start()
        processor_basic.on_sample('not valid json')
        time.sleep(0.1)
        captured = capsys.readouterr()
        assert 'Error processing sample' in captured.out

    def test_start_spawns_daemon_thread(self, processor_basic):
        processor_basic.start()
        assert processor_basic.thread.is_alive()
        assert processor_basic.thread.daemon

    def test_process_multiple_samples(self, processor_with_file, file_config):
        processor_with_file.start()
        for i in range(5):
            processor_with_file.on_sample(json.dumps({'i': i}))
        time.sleep(0.2)
        with open(file_config['save_to_file']) as f:
            lines = f.readlines()
        assert len(lines) == 5