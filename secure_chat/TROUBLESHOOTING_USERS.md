# Troubleshooting Guide - Users Not Loading

## Problem: Clicking on users in the selection screen does nothing

### Common Causes and Solutions

---

## 1. **No Users in Firestore Database**

This is the MOST COMMON issue. If you just registered and there are no other users, you'll see "No users available".

**Solution:**
- Create at least 2 user accounts first
- Sign up with email: `user1@example.com` / password: `password123`
- Sign up with email: `user2@example.com` / password: `password123`
- Log out and log back in with one of them
- Then click the "+" button to see the other user

---

## 2. **Firestore Security Rules Not Applied**

The database might be rejecting read requests due to security rules.

**Solution:**
1. Go to **Firebase Console** → **Cloud Firestore** → **Rules**
2. Make sure these rules are published:

```firestore
rules_version = '2';
service cloud.firestore {
  match /databases/{database}/documents {
    match /users/{userId} {
      allow read: if request.auth != null;
      allow create: if request.auth != null && request.auth.uid == userId;
      allow update, delete: if request.auth != null && request.auth.uid == userId;
    }
    
    match /chatRooms/{chatRoomId} {
      allow read: if request.auth != null && 
                     request.auth.uid in resource.data.members;
      allow create: if request.auth != null;
      allow update: if request.auth != null && 
                       request.auth.uid in resource.data.members;
      
      match /messages/{messageId} {
        allow read: if request.auth != null && 
                       get(/databases/$(database)/documents/chatRooms/$(chatRoomId)).data.members.hasAny([request.auth.uid]);
        allow create: if request.auth != null && 
                         request.auth.uid == request.resource.data.senderId &&
                         get(/databases/$(database)/documents/chatRooms/$(chatRoomId)).data.members.hasAny([request.auth.uid]);
      }
    }
  }
}
```

3. Click **Publish** button

---

## 3. **Check Firestore Users Collection Structure**

Verify your users collection has the correct structure:

1. Go to **Firebase Console** → **Cloud Firestore**
2. Look for a `users` collection
3. Each user document should have:
   ```
   uid: "user_id_string"
   email: "user@example.com"
   displayName: "User Name"
   createdAt: timestamp
   ```

**If the users collection doesn't exist:**
- Create it manually:
  1. Click **+ Start collection**
  2. Collection ID: `users`
  3. Click **Auto-generate ID** or enter a test user ID
  4. Add these fields:
     - `uid`: string → your_user_id
     - `email`: string → test@example.com
     - `displayName`: string → Test User
     - `createdAt`: timestamp → current date

---

## 4. **Enable Console Logging for Debugging**

To see detailed error messages:

1. Open your browser's **Developer Tools** (F12)
2. Go to **Console** tab
3. Try clicking on a user
4. Check for error messages like:
   - "No users available"
   - "Firebase error"
   - Network errors

---

## 5. **Test the Chat Room Creation**

Once you select a user, the app should:
1. Create a chat room (if it doesn't exist)
2. Navigate to the chat screen

**If nothing happens:**
- Check browser console for errors
- Make sure your Firebase config in `firebase_options.dart` is correct

---

## 6. **Verify Firebase Configuration**

Make sure your `firebase_options.dart` has the correct values from Firebase Console:

1. Go to **Firebase Console** → **Project Settings** (gear icon)
2. Your Web app configuration should show:
   - `apiKey`
   - `appId`
   - `projectId`
   - `authDomain`
   - `databaseURL`
   - `storageBucket`
   - `measurementId`

3. Update these in `lib/firebase_options.dart`

---

## 7. **Step-by-Step Testing**

Follow these steps to verify everything works:

### Step 1: Sign up two users
```
User 1: alice@example.com / password123
User 2: bob@example.com / password123
```

### Step 2: Verify users in Firestore
- Go to Firebase Console → Cloud Firestore
- Check that `users` collection has both user documents
- Each should have `uid`, `email`, `displayName`, `createdAt`

### Step 3: Test the app
- Open app in browser
- Log in as User 1
- Go to Home (Chats) - should say "No chats yet"
- Click **+** button
- Should see "Bob" in the users list
- Click on "Bob"
- Should navigate to chat screen

### Step 4: Send a message
- Type a message
- Click send
- Message should appear in the chat

### Step 5: Check from other user
- Log out from User 1
- Log in as User 2 (Bob)
- Should see a chat with "Alice"
- Message should appear in the conversation

---

## 8. **If Still Not Working - Debug Mode**

Add this to your `users_screen.dart` temporarily to see what's happening:

Replace the `_handleUserTap` method with:

```dart
Future<void> _handleUserTap(Map<String, dynamic> user) async {
  if (_isLoading) return;
  
  setState(() => _isLoading = true);

  try {
    print('=== DEBUG: Handling user tap ===');
    print('Current User ID: $_currentUserId');
    print('Current User Name: $_currentUserName');
    print('Selected User: $user');

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

    print('Chat Room ID: $chatRoomId');
    print('Attempting navigation...');

    if (mounted) {
      Navigator.of(context).pushReplacementNamed(
        '/chat',
        arguments: {
          'chatRoomId': chatRoomId,
          'otherUserId': user['uid'],
          'otherUserName': user['displayName'],
        },
      );
      print('Navigation successful');
    }
  } catch (e) {
    print('=== ERROR: $e ===');
    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Error: ${e.toString()}')),
      );
      setState(() => _isLoading = false);
    }
  }
}
```

Then:
1. Open browser Developer Tools (F12)
2. Go to Console tab
3. Click on a user
4. Look for debug output and error messages
5. Share the console output to debug further

---

## Quick Checklist

- [ ] At least 2 user accounts created
- [ ] Users collection exists in Firestore
- [ ] Firebase Security Rules are published
- [ ] `firebase_options.dart` has correct Firebase config
- [ ] Browser console shows no errors
- [ ] App is running on web (`flutter run -d web`)
- [ ] Signed in with one account
- [ ] Clicked + button
- [ ] Clicked on another user

---

## Still Need Help?

If the problem persists:
1. Check the browser console (F12) for error messages
2. Go to Firebase Console and verify:
   - Authentication is enabled
   - Cloud Firestore exists
   - Security Rules are published
   - Users collection has documents
3. Try logging out and back in
4. Try `flutter clean` → `flutter pub get` → `flutter run -d web`

