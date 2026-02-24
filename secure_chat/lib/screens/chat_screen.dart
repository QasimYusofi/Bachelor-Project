import 'package:flutter/material.dart';
import 'package:cloud_firestore/cloud_firestore.dart';
import 'package:secure_chat/services/firebase_service.dart';
import 'package:secure_chat/services/serial_service.dart';
import 'package:flutter_serial_communication/models/device_info.dart';

class ChatScreen extends StatefulWidget {
  final String chatRoomId;
  final String otherUserId;
  final String otherUserName;

  const ChatScreen({
    Key? key,
    required this.chatRoomId,
    required this.otherUserId,
    required this.otherUserName,
  }) : super(key: key);

  @override
  State<ChatScreen> createState() => _ChatScreenState();
}

class _ChatScreenState extends State<ChatScreen> {
  final _messageController = TextEditingController();
  final _firebaseService = FirebaseService();
  final _serialService = SerialService();
  late String _currentUserId;
  late String _currentUserName;
  bool _serialConnected = false;
  List<DeviceInfo> _availableDevices = [];
  Set<String> _processedMessageIds = {}; // Track processed messages to avoid duplicates

  @override
  void initState() {
    super.initState();
    try {
      _currentUserId = _firebaseService.getCurrentUser()?.uid ?? '';
      _currentUserName = _firebaseService.getCurrentUser()?.displayName ?? '';
      print('Chat Screen Initialized - Room: ${widget.chatRoomId}');
    } catch (e) {
      print('ChatScreen initState error: $e');
      _currentUserId = '';
      _currentUserName = '';
    }

    // Setup serial communication listeners
    _serialService.setupListeners(
      onMessageReceived: _handleSerialMessage,
      onConnectionChanged: (isConnected) {
        setState(() {
          _serialConnected = isConnected;
        });
        if (!isConnected) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('USB Device Disconnected')),
          );
        }
      },
    );
  }

  void _handleSerialMessage(String message) {
    // When serial message is received, send it directly to chat
    if (message.isNotEmpty) {
      _sendMessageFromSerial(message);
    }
  }

  void _sendMessageFromSerial(String message) async {
    try {
      await _firebaseService.sendMessage(
        chatRoomId: widget.chatRoomId,
        message: message,
        senderId: _currentUserId,
        senderName: _currentUserName,
      );
    } catch (e) {
      print('Error sending serial message: $e');
    }
  }

  void _sendReceivedMessageToSerial(String message, String senderName) async {
    if (!_serialConnected) return;

    try {
      // Format the message to send to serial device
      String formattedMessage = '$senderName: $message';
      bool success = await _serialService.sendMessage(formattedMessage);
      if (success) {
        print('Message sent to serial device: $formattedMessage');
      } else {
        print('Failed to send message to serial device');
      }
    } catch (e) {
      print('Error sending received message to serial: $e');
    }
  }

  void _sendMessage() async {
    if (_messageController.text.isEmpty) return;

    try {
      await _firebaseService.sendMessage(
        chatRoomId: widget.chatRoomId,
        message: _messageController.text.trim(),
        senderId: _currentUserId,
        senderName: _currentUserName,
      );
      _messageController.clear();
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Failed to send message: $e')),
        );
      }
    }
  }

  Future<void> _connectToUSB() async {
    try {
      // Get available devices
      final devices = await _serialService.getAvailableDevices();

      if (devices.isEmpty) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('No USB devices found')),
          );
        }
        return;
      }

      setState(() {
        _availableDevices = devices;
      });

      // Show device selection dialog
      if (mounted) {
        _showDeviceSelectionDialog();
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Error: $e')),
        );
      }
    }
  }

  void _showDeviceSelectionDialog() {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Select USB Device'),
        content: SizedBox(
          width: double.maxFinite,
          child: ListView.builder(
            shrinkWrap: true,
            itemCount: _availableDevices.length,
            itemBuilder: (context, index) {
              final device = _availableDevices[index];
              return ListTile(
                title: Text(device.productName),
                onTap: () {
                  Navigator.pop(context);
                  _connectToDevice(device);
                },
              );
            },
          ),
        ),
        actions: [
          TextButton(
            onPressed: () {
              Navigator.pop(context);
            },
            child: const Text('Cancel'),
          ),
        ],
      ),
    );
  }

  Future<void> _connectToDevice(DeviceInfo device) async {
    try {
      final isConnected = await _serialService.connect(device);

      if (isConnected) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(content: Text('Connected to ${device.productName}')),
          );
        }
        setState(() {
          _serialConnected = true;
        });
      } else {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(content: Text('Failed to connect to ${device.productName}')),
          );
        }
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Connection error: $e')),
        );
      }
    }
  }

  Future<void> _disconnectUSB() async {
    try {
      await _serialService.disconnect();
      setState(() {
        _serialConnected = false;
      });
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('USB Device Disconnected')),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Disconnect error: $e')),
        );
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return PopScope(
      canPop: true,
      onPopInvoked: (didPop) async {
        if (_serialConnected) {
          await _disconnectUSB();
        }
        if (didPop) return;
        if (mounted) {
          Navigator.of(context).pushReplacementNamed('/home');
        }
      },
      child: Scaffold(
        appBar: AppBar(
          title: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(widget.otherUserName),
              if (_serialConnected)
                Text(
                  'USB Connected: ${_serialService.selectedDevice?.productName ?? 'Unknown'}',
                  style: const TextStyle(fontSize: 12, fontWeight: FontWeight.normal),
                ),
            ],
          ),
          leading: IconButton(
            icon: const Icon(Icons.arrow_back),
            onPressed: () {
              if (_serialConnected) {
                _disconnectUSB();
              }
              Navigator.of(context).pushReplacementNamed('/home');
            },
          ),
        ),
        body: Column(
          children: [
            Expanded(
              child: StreamBuilder<QuerySnapshot>(
                stream: _firebaseService.getMessages(widget.chatRoomId),
                builder: (context, snapshot) {
                  if (snapshot.connectionState == ConnectionState.waiting) {
                    return const Center(child: CircularProgressIndicator());
                  }

                  if (snapshot.hasError) {
                    return Center(
                      child: Text('Error loading messages: ${snapshot.error}'),
                    );
                  }

                  if (!snapshot.hasData || snapshot.data!.docs.isEmpty) {
                    return const Center(
                      child: Text('No messages yet. Start the conversation!'),
                    );
                  }

                  List<DocumentSnapshot> messages = snapshot.data!.docs;

                  return ListView.builder(
                    reverse: true,
                    itemCount: messages.length,
                    itemBuilder: (context, index) {
                      DocumentSnapshot message = messages[index];
                      bool isMe = message['senderId'] == _currentUserId;

                      // Check if the message is already processed
                      if (_processedMessageIds.contains(message.id)) {
                        return SizedBox.shrink(); // Skip rendering but continue
                      }

                      // Add the message ID to the processed set
                      _processedMessageIds.add(message.id);

                      // Send received messages from other users to serial device
                      if (!isMe) {
                        Future.microtask(() {
                          _sendReceivedMessageToSerial(
                            message['message'] ?? '',
                            message['senderName'] ?? 'Unknown',
                          );
                        });
                      }

                      return Align(
                        alignment:
                            isMe ? Alignment.centerRight : Alignment.centerLeft,
                        child: Container(
                          margin: const EdgeInsets.symmetric(
                            vertical: 4,
                            horizontal: 8,
                          ),
                          padding: const EdgeInsets.symmetric(
                            vertical: 8,
                            horizontal: 12,
                          ),
                          decoration: BoxDecoration(
                            color: isMe ? Colors.blue : Colors.grey[300],
                            borderRadius: BorderRadius.circular(12),
                          ),
                          child: Column(
                            crossAxisAlignment: isMe
                                ? CrossAxisAlignment.end
                                : CrossAxisAlignment.start,
                            children: [
                              if (!isMe)
                                Text(
                                  message['senderName'] ?? 'Unknown',
                                  style: const TextStyle(
                                    fontWeight: FontWeight.bold,
                                    fontSize: 12,
                                  ),
                                ),
                              Text(
                                message['message'] ?? '',
                                style: TextStyle(
                                  color: isMe ? Colors.white : Colors.black,
                                ),
                              ),
                            ],
                          ),
                        ),
                      );
                    },
                  );
                },
              ),
            ),
            Padding(
              padding: const EdgeInsets.all(8.0),
              child: Row(
                children: [
                  Expanded(
                    child: TextField(
                      controller: _messageController,
                      enabled: !_serialConnected,
                      decoration: InputDecoration(
                        hintText: _serialConnected
                            ? 'USB Connected - Messages auto-sent from device'
                            : 'Type a message...',
                        border: OutlineInputBorder(
                          borderRadius: BorderRadius.circular(20),
                        ),
                        contentPadding: const EdgeInsets.symmetric(
                          horizontal: 16,
                          vertical: 8,
                        ),
                      ),
                    ),
                  ),
                  const SizedBox(width: 8),
                  // USB Connection Button
                  FloatingActionButton(
                    mini: true,
                    onPressed: _serialConnected ? _disconnectUSB : _connectToUSB,
                    backgroundColor:
                        _serialConnected ? Colors.green : Colors.orange,
                    tooltip: _serialConnected ? 'Disconnect USB' : 'Connect USB',
                    child: Icon(
                      _serialConnected ? Icons.usb_off : Icons.usb,
                    ),
                  ),
                  const SizedBox(width: 8),
                  // Send Message Button
                  FloatingActionButton(
                    mini: true,
                    onPressed: _serialConnected ? null : _sendMessage,
                    backgroundColor: _serialConnected
                        ? Colors.grey
                        : Colors.blue,
                    tooltip: _serialConnected
                        ? 'Disabled when USB connected'
                        : 'Send message',
                    child: Icon(
                      Icons.send,
                      color: _serialConnected ? Colors.grey[600] : Colors.white,
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  @override
  void dispose() {
    _messageController.dispose();
    _serialService.dispose();
    super.dispose();
  }
}
