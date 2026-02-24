import 'package:flutter/material.dart';
import 'package:secure_chat/services/firebase_service.dart';

class UsersScreen extends StatefulWidget {
  const UsersScreen({Key? key}) : super(key: key);

  @override
  State<UsersScreen> createState() => _UsersScreenState();
}

class _UsersScreenState extends State<UsersScreen> {
  final _firebaseService = FirebaseService();
  late String _currentUserId;
  late String _currentUserName;
  bool _isLoading = false;

  @override
  void initState() {
    super.initState();
    _currentUserId = _firebaseService.getCurrentUser()?.uid ?? '';
    _currentUserName = _firebaseService.getCurrentUser()?.displayName ?? '';
  }

  Future<void> _handleUserTap(Map<String, dynamic> user) async {
    if (_isLoading) return;

    setState(() => _isLoading = true);

    try {
      await _firebaseService.createOrGetChatRoom(
        userId1: _currentUserId,
        userId2: user['uid'],
        user1Name: _currentUserName,
        user2Name: user['displayName'],
      );

      String chatRoomId = _firebaseService.getChatRoomId(
        _currentUserId,
        user['uid'],
      );

      if (mounted) {
        Navigator.of(context).pushReplacementNamed(
          '/chat',
          arguments: {
            'chatRoomId': chatRoomId,
            'otherUserId': user['uid'],
            'otherUserName': user['displayName'],
          },
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Error: ${e.toString()}')),
        );
        setState(() => _isLoading = false);
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Select User')),
      body: _isLoading
          ? const Center(child: CircularProgressIndicator())
          : FutureBuilder<List<Map<String, dynamic>>>(
              future: _firebaseService.getAllUsers(_currentUserId),
              builder: (context, snapshot) {
                if (snapshot.connectionState == ConnectionState.waiting) {
                  return const Center(child: CircularProgressIndicator());
                }

                if (snapshot.hasError) {
                  return Center(
                    child: Text('Error: ${snapshot.error}'),
                  );
                }

                if (!snapshot.hasData || snapshot.data!.isEmpty) {
                  return const Center(child: Text('No users available'));
                }

                List<Map<String, dynamic>> users = snapshot.data!;

                return ListView.builder(
                  itemCount: users.length,
                  itemBuilder: (context, index) {
                    Map<String, dynamic> user = users[index];
                    return ListTile(
                      title: Text(user['displayName'] ?? 'Unknown'),
                      subtitle: Text(user['email'] ?? ''),
                      trailing: const Icon(Icons.arrow_forward_ios),
                      onTap: () => _handleUserTap(user),
                    );
                  },
                );
              },
            ),
    );
  }
}
