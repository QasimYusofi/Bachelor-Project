import 'package:flutter_serial_communication/flutter_serial_communication.dart';
import 'package:flutter_serial_communication/models/device_info.dart';
import 'dart:convert';
import 'dart:typed_data';

class SerialService {
  static final SerialService _instance = SerialService._internal();

  factory SerialService() {
    return _instance;
  }

  SerialService._internal();

  final _flutterSerialCommunication = FlutterSerialCommunication();
  bool _isConnected = false;
  DeviceInfo? _selectedDevice;
  String _serialBuffer = '';

  // Callbacks for listeners
  Function(String)? _onMessageReceived;
  Function(bool)? _onConnectionChanged;

  bool get isConnected => _isConnected;
  DeviceInfo? get selectedDevice => _selectedDevice;

  // Set up listeners
  void setupListeners({
    required Function(String) onMessageReceived,
    required Function(bool) onConnectionChanged,
  }) {
    try {
      _onMessageReceived = onMessageReceived;
      _onConnectionChanged = onConnectionChanged;

      // Listen to incoming messages
      _flutterSerialCommunication
          .getSerialMessageListener()
          .receiveBroadcastStream()
          .listen((event) {
        _handleIncomingData(event);
      }, onError: (error) {
        print('Serial message listener error: $error');
      });

      // Listen to connection changes
      _flutterSerialCommunication
          .getDeviceConnectionListener()
          .receiveBroadcastStream()
          .listen((event) {
        // Defensive handling: the plugin may emit different types (bool, Map, null)
        // Log the raw event for debugging.
        print('Device connection event: $event (type: ${event.runtimeType})');

        bool newConnectedState = false;

        if (event is bool) {
          newConnectedState = event;
        } else if (event == null) {
          newConnectedState = false;
        } else if (event is Map) {
          // If a Map is emitted, assume it's a connect event with device info
          // Treat this as connected. If the plugin uses a different shape, adjust later.
          newConnectedState = true;
        } else if (event is int) {
          // Some plugins use integer codes (1/0)
          newConnectedState = event != 0;
        } else {
          // Fallback: try to parse a truthy string
          try {
            final s = event.toString().toLowerCase();
            if (s == 'true' || s == '1') newConnectedState = true;
          } catch (_) {
            newConnectedState = false;
          }
        }

        _isConnected = newConnectedState;

        if (!_isConnected) {
          _selectedDevice = null;
          _serialBuffer = '';
        }

        _onConnectionChanged?.call(_isConnected);
      }, onError: (error) {
        print('Device connection listener error: $error');
      });
    } catch (e) {
      print('Error setting up serial listeners: $e');
    }
  }

  void _handleIncomingData(dynamic event) {
    String chunk = '';
    if (event is List) {
      try {
        chunk = utf8.decode(List<int>.from(event));
      } catch (e) {
        try {
          chunk = String.fromCharCodes(List<int>.from(event));
        } catch (e2) {
          chunk = event.toString();
        }
      }
    } else {
      chunk = event.toString();
    }

    _serialBuffer += chunk;
    _processBuffer();
  }

  void _processBuffer() {
    int newlineIndex = _serialBuffer.indexOf('\n');

    while (newlineIndex != -1) {
      String completeMessage = _serialBuffer.substring(0, newlineIndex);
      completeMessage = completeMessage.replaceAll('\r', '').trim();

      _serialBuffer = _serialBuffer.substring(newlineIndex + 1);

      if (completeMessage.isNotEmpty) {
        _onMessageReceived?.call(completeMessage);
      }

      newlineIndex = _serialBuffer.indexOf('\n');
    }
  }

  // Get available USB devices
  Future<List<DeviceInfo>> getAvailableDevices() async {
    try {
      return await _flutterSerialCommunication.getAvailableDevices();
    } catch (e) {
      print('Error getting available devices: $e');
      return [];
    }
  }

  // Connect to a device
  Future<bool> connect(DeviceInfo deviceInfo) async {
    try {
      bool isConnectionSuccess =
          await _flutterSerialCommunication.connect(deviceInfo, 115200);

      // If the connect call returned true, update local state and notify listeners.
      if (isConnectionSuccess) {
        _selectedDevice = deviceInfo;
        _serialBuffer = '';
        _isConnected = true; // proactively set connected
        _onConnectionChanged?.call(true);

        // Wait a short moment for native side to be ready before sending any test bytes
        await Future.delayed(const Duration(milliseconds: 200));

        // Send an initial test payload (optional). Keep this non-blocking for connect callers.
        try {
          final bytes = Uint8List.fromList(utf8.encode("connected\n"));
          bool isMessageSent = await _attemptWrite(bytes);
          print('Initial test message sent: $isMessageSent');
        } catch (e) {
          print('Initial test message write failed (non-fatal): $e');
        }
      } else {
        // If connect reported false, ensure internal state reflects that
        _isConnected = false;
        _selectedDevice = null;
      }

      return isConnectionSuccess;
    } catch (e) {
      print('Error connecting to device: $e');
      _isConnected = false;
      _selectedDevice = null;
      return false;
    }
  }

  // Disconnect from device
  Future<void> disconnect() async {
    try {
      await _flutterSerialCommunication.disconnect();
      _isConnected = false;
      _selectedDevice = null;
      _serialBuffer = '';
      _onConnectionChanged?.call(false);
    } catch (e) {
      print('Error disconnecting: $e');
    }
  }

  // Helper: print bytes as hex
  String _bytesToHex(Uint8List bytes) {
    return bytes.map((b) => b.toRadixString(16).padLeft(2, '0')).join(' ');
  }

  // Attempt to write bytes with retries and fallbacks
  Future<bool> _attemptWrite(Uint8List bytes) async {
    try {
      print('Attempting write (${bytes.length} bytes): ${_bytesToHex(bytes)}');
      bool result = await _flutterSerialCommunication.write(bytes);
      print('Write returned: $result');

      if (result) return true;

      // Retry once with CRLF appended (some devices expect CRLF)
      final withCrLf = Uint8List.fromList(bytes + Uint8List.fromList([13, 10]));
      print('Retrying with CRLF: ${_bytesToHex(withCrLf)}');
      result = await _flutterSerialCommunication.write(withCrLf);
      print('Write with CRLF returned: $result');
      if (result) return true;

      // Last resort: send bytes one-by-one
      print('Attempting byte-by-byte write');
      for (var b in bytes) {
        final single = Uint8List.fromList([b]);
        bool r = await _flutterSerialCommunication.write(single);
        if (!r) {
          print('Byte write failed for ${b.toRadixString(16)}');
          return false;
        }
      }

      return true;
    } catch (e) {
      print('Write attempt error: $e');
      return false;
    }
  }

  // Send message to serial device
  Future<bool> sendMessage(String message) async {
    try {
      String messageToSend = message.endsWith('\n') ? message : '$message\n';
      print('SerialService: sending -> "$messageToSend" (connected: $_isConnected)');

      // If not marked connected, attempt to continue but warn
      if (!_isConnected) {
        print('Warning: attempting to write while SerialService reports not connected. The plugin may still send bytes if native state is connected.');
      }

      final bytes = Uint8List.fromList(utf8.encode(messageToSend));
      bool isMessageSent = await _attemptWrite(bytes);

      print('SerialService: final write result -> $isMessageSent');
      return isMessageSent;
    } catch (e) {
      print('SerialService: write error -> $e');
      return false;
    }
  }

  // Cleanup
  void dispose() {
    if (_isConnected) {
      disconnect();
    }
  }
}
