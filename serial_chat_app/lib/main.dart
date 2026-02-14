import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_serial_communication/flutter_serial_communication.dart';
import 'package:flutter_serial_communication/models/device_info.dart';
import 'dart:convert';

void main() {
  runApp(const MyApp());
}

class Message {
  final String text;
  final bool isSent;
  final DateTime timestamp;

  Message({
    required this.text,
    required this.isSent,
    required this.timestamp,
  });
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  final _flutterSerialCommunicationPlugin = FlutterSerialCommunication();
  bool isConnected = false;
  List<DeviceInfo> connectedDevices = [];
  List<Message> messages = [];
  final TextEditingController _messageController = TextEditingController();
  DeviceInfo? selectedDevice;
  final ScrollController _scrollController = ScrollController();

  // Buffer to accumulate incoming data
  String _serialBuffer = '';
  bool _isReceiving = false;

  @override
  void initState() {
    super.initState();

    _flutterSerialCommunicationPlugin
        .getSerialMessageListener()
        .receiveBroadcastStream()
        .listen((event) {
      debugPrint("Received From Native: $event");

      // Convert byte array to string
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

      // Accumulate the chunk
      _serialBuffer += chunk;

      // Process complete messages (ending with newline)
      _processSerialBuffer();

      _scrollToBottom();
    });

    _flutterSerialCommunicationPlugin
        .getDeviceConnectionListener()
        .receiveBroadcastStream()
        .listen((event) {
      setState(() {
        isConnected = event;
        if (!isConnected) {
          selectedDevice = null;
          messages.clear();
          _serialBuffer = ''; // Clear buffer on disconnect
        }
      });
    });
  }

  void _processSerialBuffer() {
    // Look for newline to indicate end of message
    int newlineIndex = _serialBuffer.indexOf('\n');

    while (newlineIndex != -1) {
      // Extract the complete message
      String completeMessage = _serialBuffer.substring(0, newlineIndex);

      // Remove any carriage return
      completeMessage = completeMessage.replaceAll('\r', '');

      // Remove the processed message from buffer
      _serialBuffer = _serialBuffer.substring(newlineIndex + 1);

      // Trim and process the message
      completeMessage = completeMessage.trim();

      if (completeMessage.isNotEmpty) {
        setState(() {
          messages.add(Message(
            text: completeMessage,
            isSent: false,
            timestamp: DateTime.now(),
          ));
        });

        // Log for debugging
        debugPrint("Complete message received (${completeMessage.length} chars): $completeMessage");
        if (completeMessage.length > 50) {
          debugPrint("First 50 chars: ${completeMessage.substring(0, 50)}...");
        }
      }

      // Check for more complete messages
      newlineIndex = _serialBuffer.indexOf('\n');
    }
  }

  void _scrollToBottom() {
    Future.delayed(const Duration(milliseconds: 100), () {
      if (_scrollController.hasClients) {
        _scrollController.animateTo(
          _scrollController.position.maxScrollExtent,
          duration: const Duration(milliseconds: 300),
          curve: Curves.easeOut,
        );
      }
    });
  }

  _getAllConnectedDevicedButtonPressed() async {
    List<DeviceInfo> newConnectedDevices =
    await _flutterSerialCommunicationPlugin.getAvailableDevices();
    setState(() {
      connectedDevices = newConnectedDevices;
    });
  }

  _connectButtonPressed(DeviceInfo deviceInfo) async {
    bool isConnectionSuccess =
    await _flutterSerialCommunicationPlugin.connect(deviceInfo, 115200);
    debugPrint("Is Connection Success: $isConnectionSuccess");
    if (isConnectionSuccess) {
      setState(() {
        selectedDevice = deviceInfo;
        messages.clear();
        _serialBuffer = ''; // Clear buffer on new connection
      });
    }
  }

  _disconnectButtonPressed() async {
    await _flutterSerialCommunicationPlugin.disconnect();
  }

  _sendMessageButtonPressed() async {
    String messageText = _messageController.text.trim();
    if (messageText.isEmpty) return;

    setState(() {
      messages.add(Message(
        text: messageText,
        isSent: true,
        timestamp: DateTime.now(),
      ));
    });

    _messageController.clear();
    _scrollToBottom();

    // Add newline character to indicate end of message
    String messageToSend = messageText + '\n';

    bool isMessageSent = await _flutterSerialCommunicationPlugin
        .write(Uint8List.fromList(utf8.encode(messageToSend)));

    debugPrint("Is Message Sent: $isMessageSent");
    if (isMessageSent) {
      debugPrint("Sent message (${messageText.length} chars): $messageText");
    }
  }

  @override
  void dispose() {
    _messageController.dispose();
    _scrollController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      home: Scaffold(
        appBar: AppBar(
          title: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Text('Serial Chat'),
              Text(
                selectedDevice != null
                    ? selectedDevice!.productName
                    : 'No device connected',
                style: const TextStyle(fontSize: 12, fontWeight: FontWeight.normal),
              ),
            ],
          ),
          actions: [
            if (isConnected && selectedDevice != null)
              Padding(
                padding: const EdgeInsets.all(16.0),
                child: Center(
                  child: FilledButton.tonal(
                    onPressed: _disconnectButtonPressed,
                    child: const Text('Disconnect'),
                  ),
                ),
              ),
          ],
        ),
        body: isConnected && selectedDevice != null
            ? Column(
          children: [
            Expanded(
              child: messages.isEmpty
                  ? Center(
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Text(
                      'No messages yet',
                      style: Theme.of(context).textTheme.bodyLarge,
                    ),
                    const SizedBox(height: 8),
                    Text(
                      'Buffer: ${_serialBuffer.length} chars',
                      style: Theme.of(context).textTheme.bodySmall,
                    ),
                  ],
                ),
              )
                  : ListView.builder(
                controller: _scrollController,
                itemCount: messages.length,
                itemBuilder: (context, index) {
                  // Skip first received message
                  if (index == 0 && messages[index].isSent == false) {
                    return const SizedBox.shrink();
                  }

                  final message = messages[index];
                  bool isHex = _isHexString(message.text);

                  return Align(
                    alignment: message.isSent
                        ? Alignment.centerRight
                        : Alignment.centerLeft,
                    child: GestureDetector(
                      onTap: message.isSent
                          ? null
                          : () {
                        Clipboard.setData(
                          ClipboardData(text: message.text),
                        );
                        ScaffoldMessenger.of(context)
                            .showSnackBar(
                          const SnackBar(
                            content:
                            Text('Copied to clipboard'),
                            duration:
                            Duration(milliseconds: 1500),
                          ),
                        );
                      },
                      child: Container(
                        margin: const EdgeInsets.symmetric(
                          horizontal: 16.0,
                          vertical: 8.0,
                        ),
                        padding: const EdgeInsets.symmetric(
                          horizontal: 16.0,
                          vertical: 12.0,
                        ),
                        constraints: BoxConstraints(
                          maxWidth:
                          MediaQuery.of(context).size.width *
                              0.7,
                        ),
                        decoration: BoxDecoration(
                          color: message.isSent
                              ? Colors.blue
                              : (isHex ? Colors.purple[300] : Colors.grey[300]),
                          borderRadius:
                          BorderRadius.circular(12.0),
                        ),
                        child: Row(
                          mainAxisSize: MainAxisSize.min,
                          children: [
                            Flexible(
                              child: Column(
                                crossAxisAlignment:
                                CrossAxisAlignment.start,
                                children: [
                                  Text(
                                    message.text,
                                    style: TextStyle(
                                      color: message.isSent
                                          ? Colors.white
                                          : Colors.black,
                                      fontSize: 14,
                                    ),
                                  ),
                                  const SizedBox(height: 4.0),
                                  Row(
                                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                                    children: [
                                      Text(
                                        "${message.timestamp.hour.toString().padLeft(2, '0')}:${message.timestamp.minute.toString().padLeft(2, '0')}",
                                        style: TextStyle(
                                          fontSize: 11.0,
                                          color: message.isSent
                                              ? Colors.white70
                                              : Colors.grey[600],
                                        ),
                                      ),
                                      if (isHex)
                                        Text(
                                          'Encrypted (${message.text.length ~/ 2} bytes)',
                                          style: TextStyle(
                                            fontSize: 10.0,
                                            color: message.isSent
                                                ? Colors.white70
                                                : Colors.grey[600],
                                          ),
                                        ),
                                    ],
                                  ),
                                ],
                              ),
                            ),
                            if (!message.isSent) ...[
                              const SizedBox(width: 8.0),
                              Icon(
                                Icons.content_copy,
                                size: 16,
                                color: Colors.grey[600],
                              ),
                            ],
                          ],
                        ),
                      ),
                    ),
                  );
                },
              ),
            ),
            Padding(
              padding: const EdgeInsets.all(16.0),
              child: Column(
                children: [
                  if (_serialBuffer.isNotEmpty)
                    Padding(
                      padding: const EdgeInsets.only(bottom: 8.0),
                      child: Text(
                        'Buffer: ${_serialBuffer.length} chars',
                        style: Theme.of(context).textTheme.bodySmall,
                      ),
                    ),
                  Row(
                    children: [
                      Expanded(
                        child: TextField(
                          controller: _messageController,
                          maxLines: null,
                          decoration: InputDecoration(
                            hintText: 'Type a message...',
                            border: OutlineInputBorder(
                              borderRadius: BorderRadius.circular(8.0),
                            ),
                            contentPadding: const EdgeInsets.symmetric(
                              horizontal: 16.0,
                              vertical: 12.0,
                            ),
                          ),
                          onSubmitted: (_) => _sendMessageButtonPressed(),
                        ),
                      ),
                      const SizedBox(width: 8.0),
                      FilledButton.icon(
                        onPressed: _sendMessageButtonPressed,
                        icon: const Icon(Icons.send),
                        label: const Text('Send'),
                      ),
                    ],
                  ),
                ],
              ),
            ),
          ],
        )
            : Padding(
          padding: const EdgeInsets.all(16.0),
          child: Column(
            children: [
              Text(
                "Is Connected: $isConnected",
                style: Theme.of(context).textTheme.titleMedium,
              ),
              const SizedBox(height: 16.0),
              FilledButton.icon(
                onPressed: _getAllConnectedDevicedButtonPressed,
                icon: const Icon(Icons.usb),
                label: const Text("Get All Connected Devices"),
              ),
              const SizedBox(height: 16.0),
              if (connectedDevices.isEmpty)
                Expanded(
                  child: Center(
                    child: Text(
                      'No devices found',
                      style: Theme.of(context).textTheme.bodyLarge,
                    ),
                  ),
                )
              else
                Expanded(
                  child: ListView.builder(
                    itemCount: connectedDevices.length,
                    itemBuilder: (context, index) {
                      return Card(
                        margin: const EdgeInsets.symmetric(vertical: 8.0),
                        child: Padding(
                          padding: const EdgeInsets.all(16.0),
                          child: Row(
                            children: [
                              Flexible(
                                child: Column(
                                  crossAxisAlignment:
                                  CrossAxisAlignment.start,
                                  children: [
                                    Text(
                                      connectedDevices[index].productName,
                                      style: Theme.of(context)
                                          .textTheme
                                          .titleSmall,
                                    ),
                                    const SizedBox(height: 4.0),
                                    Text(
                                      'Connected Devices',
                                      style: Theme.of(context)
                                          .textTheme
                                          .bodySmall,
                                    ),
                                  ],
                                ),
                              ),
                              const SizedBox(width: 16.0),
                              FilledButton.icon(
                                onPressed: () {
                                  _connectButtonPressed(
                                      connectedDevices[index]);
                                },
                                icon: const Icon(Icons.link),
                                label: const Text("Connect"),
                              ),
                            ],
                          ),
                        ),
                      );
                    },
                  ),
                ),
            ],
          ),
        ),
      ),
    );
  }

  bool _isHexString(String text) {
    if (text.isEmpty) return false;
    // Check if string contains only hex characters
    final hexRegex = RegExp(r'^[0-9a-fA-F]+$');
    return hexRegex.hasMatch(text);
  }
}