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
    _currentUserId = _firebaseService.getCurrentUser()?.uid ?? '';
    _currentUserName = _firebaseService.getCurrentUser()?.displayName ?? '';
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
                Navigator.of(context).pushReplacementNamed('/login');
              }
            },
          ),
        ],
      ),
      body: StreamBuilder<QuerySnapshot>(
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
                  const Text('Error loading chats'),
                  const SizedBox(height: 16),
                  Text(snapshot.error.toString(), textAlign: TextAlign.center),
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
            return const Center(
              child: Text('No chats yet. Add a friend to start chatting!'),
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
              List<String> members = List<String>.from(chatRoom['members']);
              List<String> memberNames = List<String>.from(chatRoom['memberNames']);

              String otherUserId = members.firstWhere((id) => id != _currentUserId);
              int otherUserIndex = members.indexOf(otherUserId);
              String otherUserName = memberNames[otherUserIndex];

              return ListTile(
                title: Text(otherUserName),
                subtitle: Text(chatRoom['lastMessage'] ?? 'No messages yet'),
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
