import 'package:firebase_auth/firebase_auth.dart';
import 'package:cloud_firestore/cloud_firestore.dart';

class FirebaseService {
  final FirebaseAuth _auth = FirebaseAuth.instance;
  final FirebaseFirestore _firestore = FirebaseFirestore.instance;

  // Auth Methods
  Future<UserCredential?> signUp(String email, String password, String displayName) async {
    try {
      UserCredential userCredential = await _auth.createUserWithEmailAndPassword(
        email: email,
        password: password,
      );

      await userCredential.user?.updateDisplayName(displayName);

      // Create user document in Firestore
      await _firestore.collection('users').doc(userCredential.user?.uid).set({
        'uid': userCredential.user?.uid,
        'email': email,
        'displayName': displayName,
        'createdAt': FieldValue.serverTimestamp(),
      });

      return userCredential;
    } on FirebaseAuthException catch (e) {
      print('Sign up error: ${e.message}');
      rethrow;
    }
  }

  Future<UserCredential?> signIn(String email, String password) async {
    try {
      UserCredential userCredential = await _auth.signInWithEmailAndPassword(
        email: email,
        password: password,
      );
      return userCredential;
    } on FirebaseAuthException catch (e) {
      print('Sign in error: ${e.message}');
      rethrow;
    }
  }

  Future<void> signOut() async {
    try {
      await _auth.signOut();
    } catch (e) {
      print('Sign out error: $e');
      rethrow;
    }
  }

  // Chat Methods
  Future<void> sendMessage({
    required String chatRoomId,
    required String message,
    required String senderId,
    required String senderName,
  }) async {
    try {
      await _firestore
          .collection('chatRooms')
          .doc(chatRoomId)
          .collection('messages')
          .add({
        'message': message,
        'senderId': senderId,
        'senderName': senderName,
        'timestamp': FieldValue.serverTimestamp(),
      });

      // Update chatroom last message
      await _firestore.collection('chatRooms').doc(chatRoomId).update({
        'lastMessage': message,
        'lastMessageTime': FieldValue.serverTimestamp(),
        'lastMessageSender': senderName,
      });
    } catch (e) {
      print('Send message error: $e');
      rethrow;
    }
  }

  Stream<QuerySnapshot> getMessages(String chatRoomId) {
    return _firestore
        .collection('chatRooms')
        .doc(chatRoomId)
        .collection('messages')
        .orderBy('timestamp', descending: true)
        .snapshots();
  }

  Future<void> createOrGetChatRoom({
    required String userId1,
    required String userId2,
    required String user1Name,
    required String user2Name,
  }) async {
    try {
      String chatRoomId = getChatRoomId(userId1, userId2);

      await _firestore.collection('chatRooms').doc(chatRoomId).set({
        'members': [userId1, userId2],
        'memberNames': [user1Name, user2Name],
        'createdAt': FieldValue.serverTimestamp(),
        'lastMessage': '',
        'lastMessageTime': FieldValue.serverTimestamp(),
      }, SetOptions(merge: true));

      print('Chat room created/updated: $chatRoomId');
    } catch (e) {
      print('Create/Get chat room error: $e');
      rethrow;
    }
  }

  String getChatRoomId(String userId1, String userId2) {
    List<String> ids = [userId1, userId2];
    ids.sort();
    return ids.join('_');
  }

  Future<List<Map<String, dynamic>>> getAllUsers(String currentUserId) async {
    try {
      QuerySnapshot snapshot = await _firestore.collection('users').get();
      List<Map<String, dynamic>> users = [];

      for (var doc in snapshot.docs) {
        if (doc['uid'] != currentUserId) {
          users.add(doc.data() as Map<String, dynamic>);
        }
      }

      return users;
    } catch (e) {
      print('Get all users error: $e');
      rethrow;
    }
  }

  Stream<QuerySnapshot> getChatRooms(String userId) {
    try {
      print('Getting chat rooms for user: $userId');
      return _firestore
          .collection('chatRooms')
          .where('members', arrayContains: userId)
          .snapshots();
    } catch (e) {
      print('Get chat rooms error: $e');
      // Return empty stream instead of trying to instantiate abstract class
      return Stream.empty();
    }
  }

  // Delete a specific message
  Future<void> deleteMessage({
    required String chatRoomId,
    required String messageId,
  }) async {
    try {
      await _firestore
          .collection('chatRooms')
          .doc(chatRoomId)
          .collection('messages')
          .doc(messageId)
          .delete();
      print('Message deleted: $messageId');
    } catch (e) {
      print('Delete message error: $e');
      rethrow;
    }
  }

  // Delete entire chat room
  Future<void> deleteChatRoom(String chatRoomId) async {
    try {
      // Delete all messages in the chat room
      final messagesSnapshot = await _firestore
          .collection('chatRooms')
          .doc(chatRoomId)
          .collection('messages')
          .get();

      for (var doc in messagesSnapshot.docs) {
        await doc.reference.delete();
      }

      // Delete the chat room document
      await _firestore.collection('chatRooms').doc(chatRoomId).delete();
      print('Chat room deleted: $chatRoomId');
    } catch (e) {
      print('Delete chat room error: $e');
      rethrow;
    }
  }

  User? getCurrentUser() {
    return _auth.currentUser;
  }

  Stream<User?> get authStateChanges => _auth.authStateChanges();
}
