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
        _isConnected = event;
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
      if (isConnectionSuccess) {
        _selectedDevice = deviceInfo;
        _serialBuffer = '';
      }
      return isConnectionSuccess;
    } catch (e) {
      print('Error connecting to device: $e');
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
    } catch (e) {
      print('Error disconnecting: $e');
    }
  }

  // Send message to serial device
  Future<bool> sendMessage(String message) async {
    try {
      String messageToSend = '$message\n';
      bool isMessageSent = await _flutterSerialCommunication
          .write(Uint8List.fromList(utf8.encode(messageToSend)));
      return isMessageSent;
    } catch (e) {
      print('Error sending message: $e');
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
