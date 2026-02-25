import 'package:flutter/material.dart';
import 'package:cloud_firestore/cloud_firestore.dart';
import 'package:secure_chat/services/firebase_service.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({Key? key}) : super(key: key);

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  final _firebaseService = FirebaseService();
  late String _currentUserId;
  late String _currentUserName;

  @override
  void initState() {
    super.initState();
    try {
      _currentUserId = _firebaseService.getCurrentUser()?.uid ?? '';
      _currentUserName = _firebaseService.getCurrentUser()?.displayName ?? '';
      print('HomeScreen initialized - UserId: $_currentUserId, UserName: $_currentUserName');
    } catch (e) {
      print('HomeScreen initState error: $e');
      _currentUserId = '';
      _currentUserName = '';
    }
  }

  void _deleteChatRoom(String chatRoomId, String otherUserName) async {
    // Show a snackbar while deleting
    final snackbar = ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text('Deleting chat with $otherUserName...'),
        duration: const Duration(seconds: 2),
        behavior: SnackBarBehavior.floating,
      ),
    );

    try {
      // Wait for the snackbar to show
      await Future.delayed(const Duration(milliseconds: 500));

      // Delete the chat room
      await _firebaseService.deleteChatRoom(chatRoomId);

      // Hide the snackbar
      snackbar.close();
    } catch (e) {
      // Hide the snackbar
      snackbar.close();

      // Show error message
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Error deleting chat: $e'),
          duration: const Duration(seconds: 2),
          backgroundColor: Colors.red,
          behavior: SnackBarBehavior.floating,
        ),
      );
    }
  }

  void _showDeleteConfirmation(String chatRoomId, String otherUserName) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Delete Chat'),
        content: Text('Are you sure you want to delete the chat with $otherUserName?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('Cancel'),
          ),
          TextButton(
            onPressed: () {
              Navigator.of(context).pop();
              _deleteChatRoom(chatRoomId, otherUserName);
            },
            child: const Text('Delete', style: TextStyle(color: Colors.red)),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Chats'),
        actions: [
          IconButton(
            icon: const Icon(Icons.logout),
            onPressed: () async {
              await _firebaseService.signOut();
              if (mounted) {
                if (context.mounted) {
                  Navigator.of(context).pushReplacementNamed('/login');
                }
              }
            },
          ),
        ],
      ),
      body: _currentUserId.isEmpty
          ? const Center(
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  CircularProgressIndicator(),
                  SizedBox(height: 16),
                  Text('Loading user info...'),
                ],
              ),
            )
          : StreamBuilder<QuerySnapshot>(
              stream: _firebaseService.getChatRooms(_currentUserId),
              builder: (context, snapshot) {
                if (snapshot.connectionState == ConnectionState.waiting) {
                  return const Center(child: CircularProgressIndicator());
                }

                if (snapshot.hasError) {
                  print('Home Stream Error: ${snapshot.error}');
                  return Center(
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        const Icon(Icons.error_outline, size: 48, color: Colors.red),
                        const SizedBox(height: 16),
                        const Text('Error loading chats'),
                        const SizedBox(height: 8),
                        Padding(
                          padding: const EdgeInsets.all(16.0),
                          child: Text(
                            snapshot.error.toString(),
                            textAlign: TextAlign.center,
                            style: const TextStyle(fontSize: 12),
                          ),
                        ),
                        const SizedBox(height: 16),
                        ElevatedButton(
                          onPressed: () {
                            setState(() {});
                          },
                          child: const Text('Retry'),
                        ),
                      ],
                    ),
                  );
                }

                if (!snapshot.hasData || snapshot.data!.docs.isEmpty) {
                  return Center(
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        const Icon(Icons.chat_bubble_outline, size: 64, color: Colors.grey),
                        const SizedBox(height: 16),
                        const Text('No chats yet'),
                        const SizedBox(height: 8),
                        const Text('Add a friend to start chatting!', style: TextStyle(color: Colors.grey)),
                        const SizedBox(height: 24),
                        ElevatedButton.icon(
                          onPressed: () {
                            Navigator.of(context).pushNamed('/users');
                          },
                          icon: const Icon(Icons.person_add),
                          label: const Text('Add Friend'),
                        ),
                      ],
                    ),
                  );
                }

                // Sort chat rooms by lastMessageTime (newest first)
                List<DocumentSnapshot> chatRooms = snapshot.data!.docs.toList();
                chatRooms.sort((a, b) {
                  Timestamp? timeA = a['lastMessageTime'] as Timestamp?;
                  Timestamp? timeB = b['lastMessageTime'] as Timestamp?;
                  if (timeA == null || timeB == null) return 0;
                  return timeB.compareTo(timeA); // Descending order
                });

                print('Chat rooms found: ${chatRooms.length}');

                return ListView.builder(
                  itemCount: chatRooms.length,
                  itemBuilder: (context, index) {
                    DocumentSnapshot chatRoom = chatRooms[index];
                    List<String> members = List<String>.from(chatRoom['members'] ?? []);
                    List<String> memberNames = List<String>.from(chatRoom['memberNames'] ?? []);

                    if (members.isEmpty || memberNames.isEmpty) {
                      return const SizedBox.shrink();
                    }

                    String otherUserId = members.firstWhere(
                      (id) => id != _currentUserId,
                      orElse: () => '',
                    );

                    if (otherUserId.isEmpty) {
                      return const SizedBox.shrink();
                    }

                    int otherUserIndex = members.indexOf(otherUserId);
                    String otherUserName = otherUserIndex >= 0 && otherUserIndex < memberNames.length
                        ? memberNames[otherUserIndex]
                        : 'Unknown';

                    return Dismissible(
                      key: Key(chatRoom.id),
                      direction: DismissDirection.endToStart,
                      background: Container(
                        color: Colors.red,
                        alignment: Alignment.centerRight,
                        padding: const EdgeInsets.only(right: 20),
                        child: const Icon(Icons.delete, color: Colors.white),
                      ),
                      onDismissed: (direction) {
                        _deleteChatRoom(chatRoom.id, otherUserName);
                      },
                      child: Card(
                        margin: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                        elevation: 2,
                        child: ListTile(
                          leading: CircleAvatar(
                            child: Text(
                              otherUserName.isNotEmpty ? otherUserName[0].toUpperCase() : '?',
                            ),
                          ),
                          title: Text(
                            otherUserName,
                            style: const TextStyle(fontWeight: FontWeight.w600),
                          ),
                          subtitle: Text(
                            chatRoom['lastMessage'] ?? 'No messages yet',
                            maxLines: 1,
                            overflow: TextOverflow.ellipsis,
                          ),
                          trailing: PopupMenuButton(
                            itemBuilder: (context) => [
                              PopupMenuItem(
                                child: const Row(
                                  children: [
                                    Icon(Icons.delete, color: Colors.red),
                                    SizedBox(width: 10),
                                    Text('Delete Chat'),
                                  ],
                                ),
                                onTap: () {
                                  _showDeleteConfirmation(chatRoom.id, otherUserName);
                                },
                              ),
                            ],
                          ),
                          onTap: () {
                            Navigator.of(context).pushNamed(
                              '/chat',
                              arguments: {
                                'chatRoomId': chatRoom.id,
                                'otherUserId': otherUserId,
                                'otherUserName': otherUserName,
                              },
                            );
                          },
                        ),
                      ),
                    );
                  },
                );
              },
            ),
      floatingActionButton: FloatingActionButton(
        onPressed: () {
          Navigator.of(context).pushNamed('/users');
        },
        child: const Icon(Icons.add),
      ),
    );
  }
}
