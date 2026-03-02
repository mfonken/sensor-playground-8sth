import socket
import threading
import time
import pytest
from unittest.mock import MagicMock, patch, call
from tcp_client import TCPClient


@pytest.fixture
def client():
    return TCPClient(host='127.0.0.1', port=9999)


@pytest.fixture
def client_with_callback():
    cb = MagicMock()
    return TCPClient(host='127.0.0.1', port=9999, on_sample_callback=cb), cb


@pytest.fixture
def mock_sock():
    return MagicMock(spec=socket.socket)


# --- Helpers ---

def make_connected_client(mock_sock):
    """Return a client with a pre-injected mock socket (simulates connected state)."""
    c = TCPClient('127.0.0.1', 9999)
    c.sock = mock_sock
    return c


# --- Init ---

class TestInit:
    def test_default_delimiter(self, client):
        assert client.sample_delimiter == "\n"

    def test_custom_delimiter(self):
        c = TCPClient('localhost', 8080, sample_delimiter="|")
        assert c.sample_delimiter == "|"

    def test_initial_state(self, client):
        assert client.sock is None
        assert client.running is False
        assert client.thread is None
        assert client.on_sample_callback is None

    def test_host_port_stored(self):
        c = TCPClient('example.com', 1234)
        assert c.host == 'example.com'
        assert c.port == 1234


# --- connect ---

class TestConnect:
    def test_connect_creates_socket_and_calls_connect(self, client):
        mock_sock = MagicMock()
        with patch('socket.socket', return_value=mock_sock) as mock_socket_class:
            client.connect()
            mock_socket_class.assert_called_once_with(socket.AF_INET, socket.SOCK_STREAM)
            mock_sock.connect.assert_called_once_with(('127.0.0.1', 9999))
            assert client.sock is mock_sock

    def test_connect_is_idempotent(self, client):
        mock_sock = MagicMock()
        with patch('socket.socket', return_value=mock_sock):
            client.connect()
            client.connect()  # second call should be a no-op
            assert mock_sock.connect.call_count == 1

    def test_connect_does_not_replace_existing_socket(self, client, mock_sock):
        client.sock = mock_sock
        with patch('socket.socket') as mock_socket_class:
            client.connect()
            mock_socket_class.assert_not_called()


# --- disconnect ---

class TestDisconnect:
    def test_disconnect_closes_socket(self, mock_sock):
        client = make_connected_client(mock_sock)
        client.disconnect()
        mock_sock.close.assert_called_once()
        assert client.sock is None

    def test_disconnect_when_not_connected(self, client):
        client.disconnect()  # Should not raise
        assert client.sock is None

    def test_disconnect_sets_sock_to_none(self, mock_sock):
        client = make_connected_client(mock_sock)
        client.disconnect()
        assert client.sock is None


# --- send ---

class TestSend:
    def test_send_encodes_and_sends_message(self, mock_sock):
        client = make_connected_client(mock_sock)
        client.send("hello")
        mock_sock.sendall.assert_called_once_with(b"hello")

    def test_send_does_nothing_when_not_connected(self, client):
        client.send("hello")  # Should not raise

    def test_send_suppresses_socket_error(self, mock_sock):
        mock_sock.sendall.side_effect = OSError("broken pipe")
        client = make_connected_client(mock_sock)
        client.send("hello")  # Should not raise

    def test_send_is_thread_safe(self, mock_sock):
        client = make_connected_client(mock_sock)
        threads = [threading.Thread(target=client.send, args=(f"msg{i}",)) for i in range(10)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        assert mock_sock.sendall.call_count == 10


# --- receive ---

class TestReceive:
    def test_receive_calls_callback_for_each_sample(self, mock_sock):
        cb = MagicMock()
        client = TCPClient('127.0.0.1', 9999, on_sample_callback=cb)
        client.sock = mock_sock
        mock_sock.recv.return_value = b"sample1\nsample2\nsample3"
        client.receive()
        assert cb.call_count == 3
        cb.assert_any_call("sample1")
        cb.assert_any_call("sample2")
        cb.assert_any_call("sample3")

    def test_receive_single_sample(self, mock_sock):
        cb = MagicMock()
        client = TCPClient('127.0.0.1', 9999, on_sample_callback=cb)
        client.sock = mock_sock
        mock_sock.recv.return_value = b'{"x":1}\n'
        client.receive()
        cb.assert_called_once_with('{"x":1}')

    def test_receive_custom_delimiter(self, mock_sock):
        cb = MagicMock()
        client = TCPClient('127.0.0.1', 9999, on_sample_callback=cb, sample_delimiter="|")
        client.sock = mock_sock
        mock_sock.recv.return_value = b"a|b|c"
        client.receive()
        assert cb.call_count == 3

    def test_receive_empty_data_returns_early(self, mock_sock):
        cb = MagicMock()
        client = TCPClient('127.0.0.1', 9999, on_sample_callback=cb)
        client.sock = mock_sock
        mock_sock.recv.return_value = b""
        client.receive()
        cb.assert_not_called()

    def test_receive_no_callback(self, mock_sock):
        client = make_connected_client(mock_sock)
        mock_sock.recv.return_value = b"data\n"
        client.receive()  # Should not raise

    def test_receive_does_nothing_when_not_connected(self, client):
        client.receive()  # Should not raise

    def test_receive_suppresses_socket_exception(self, mock_sock):
        client = make_connected_client(mock_sock)
        mock_sock.recv.side_effect = OSError("connection reset")
        client.receive()  # Should not raise


# --- start / stop ---

class TestStartStop:
    def test_start_sets_running_true(self, client):
        with patch.object(client, 'connection_manager'):
            t = client.start()
            assert client.running is True

    def test_start_returns_thread(self, client):
        with patch.object(client, 'connection_manager'):
            t = client.start()
            assert isinstance(t, threading.Thread)

    def test_start_thread_is_daemon(self, client):
        with patch.object(client, 'connection_manager'):
            t = client.start()
            assert t.daemon is True

    def test_stop_sets_running_false(self, client):
        with patch.object(client, 'connection_manager'):
            client.start().start()
            client.stop()
            assert client.running is False

    def test_stop_disconnects(self, client, mock_sock):
        client.sock = mock_sock
        client.running = True
        client.thread = MagicMock()
        client.stop()
        mock_sock.close.assert_called_once()

    def test_stop_with_no_thread(self, client):
        client.running = True
        client.stop()  # Should not raise


# --- restart ---

class TestRestart:
    def test_restart_stops_and_restarts(self, client):
        stop_called = []
        original_stop = client.stop

        def mock_stop():
            stop_called.append(True)
            client.running = False

        client.stop = mock_stop
        with patch.object(client, 'connection_manager'):
            client.restart()
        assert stop_called
        assert client.running is True
        assert client.thread is not None

    def test_restart_returns_new_thread(self, client):
        client.stop = MagicMock(side_effect=lambda: setattr(client, 'running', False))
        with patch.object(client, 'connection_manager'):
            t = client.restart()
            assert isinstance(t, threading.Thread)

    def test_restart_thread_is_daemon(self, client):
        client.stop = MagicMock(side_effect=lambda: setattr(client, 'running', False))
        with patch.object(client, 'connection_manager'):
            t = client.restart()
            assert t.daemon is True


# --- connection_manager (integration) ---

class TestConnectionManager:
    def test_reconnects_after_error(self):
        cb = MagicMock()
        connect_attempts = []

        def failing_connect(self_inner):
            connect_attempts.append(1)
            if len(connect_attempts) < 3:
                raise OSError("refused")

        client = TCPClient('127.0.0.1', 9999, on_sample_callback=cb)

        with patch.object(TCPClient, 'connect', failing_connect), \
             patch.object(TCPClient, 'receive', side_effect=lambda: setattr(client, 'running', False)), \
             patch('time.sleep'):
            client.running = True
            client.connection_manager()

        assert len(connect_attempts) >= 3

    def test_connection_manager_stops_when_running_false(self):
        client = TCPClient('127.0.0.1', 9999)
        client.running = False
        # Should exit immediately without blocking
        t = threading.Thread(target=client.connection_manager)
        t.start()
        t.join(timeout=1.0)
        assert not t.is_alive()