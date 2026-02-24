# Firebase Setup Guide for Secure Chat App

## Overview
This guide provides complete instructions to configure Firebase for the Secure Chat Flutter application.

## Prerequisites
- A Google account
- Flutter SDK installed
- The chat app project set up

## Step 1: Create a Firebase Project

1. Go to [Firebase Console](https://console.firebase.google.com/)
2. Click **"Create a project"** or **"Add project"**
3. Enter your project name: `secure-chat` (or any name you prefer)
4. Click **Continue**
5. Disable Google Analytics (optional) for simplicity
6. Click **Create project**
7. Wait for the project to be created and click **Continue**

## Step 2: Enable Authentication

### Enable Email/Password Authentication:

1. In Firebase Console, go to **Authentication** (left sidebar)
2. Click on the **Sign-in method** tab
3. Click on **Email/Password**
4. Enable it by toggling the switch
5. Click **Save**

## Step 3: Set Up Cloud Firestore Database

1. Go to **Cloud Firestore** (left sidebar)
2. Click **Create database**
3. Choose your region (preferably closest to you)
4. Start in **Production mode** (we'll set security rules later)
5. Click **Create**
6. Wait for the database to be created

### Set Firestore Security Rules:

1. In Cloud Firestore, go to **Rules** tab
2. Replace the default rules with:

```firestore
rules_version = '2';
service cloud.firestore {
  match /databases/{database}/documents {
    // Users collection
    match /users/{userId} {
      allow read: if request.auth != null;
      allow create: if request.auth != null && request.auth.uid == userId;
      allow update, delete: if request.auth != null && request.auth.uid == userId;
    }
    
    // Chat rooms collection
    match /chatRooms/{chatRoomId} {
      allow read: if request.auth != null && 
                     request.auth.uid in resource.data.members;
      allow create: if request.auth != null;
      allow update: if request.auth != null && 
                       request.auth.uid in resource.data.members;
      
      // Messages subcollection
      match /messages/{messageId} {
        allow read: if request.auth != null && 
                       get(/databases/$(database)/documents/chatRooms/$(chatRoomId)).data.members.hasAny([request.auth.uid]);
        allow create: if request.auth != null && 
                         request.auth.uid == request.resource.data.senderId &&
                         get(/databases/$(database)/documents/chatRooms/$(chatRoomId)).data.members.hasAny([request.auth.uid]);
        allow delete: if request.auth != null && 
                         request.auth.uid == resource.data.senderId;
      }
    }
  }
}
```

3. Click **Publish**

## Step 4: Get Your Firebase Configuration

### For Android:

1. In Firebase Console, click **Project Settings** (gear icon)
2. Go to **Your apps** section
3. Click on your Android app (or add one if not listed)
4. Download the `google-services.json` file
5. Place it in: `android/app/google-services.json`

### For iOS:

1. In Firebase Console, click **Project Settings** (gear icon)
2. Go to **Your apps** section
3. Click on your iOS app (or add one if not listed)
4. Download the `GoogleService-Info.plist` file
5. Open iOS project in Xcode: `open ios/Runner.xcworkspace`
6. Right-click on **Runner** → **Add Files to Runner**
7. Select the downloaded `GoogleService-Info.plist`
8. Make sure it's added to the **Runner** target

### For Web:

1. In Firebase Console, click **Project Settings** (gear icon)
2. Go to **Your apps** section
3. Select your Web app (or add one)
4. Copy the Firebase configuration object

## Step 5: Update firebase_options.dart

1. Replace the placeholder values in `lib/firebase_options.dart` with your actual Firebase credentials:

From Firebase Console → Project Settings → Your apps:

```dart
// For Web:
static const FirebaseOptions web = FirebaseOptions(
  apiKey: 'YOUR_API_KEY',           // Copy from Firebase Console
  appId: 'YOUR_APP_ID',              // Copy from Firebase Console
  messagingSenderId: 'YOUR_SENDER_ID', // Copy from Firebase Console
  projectId: 'YOUR_PROJECT_ID',      // Copy from Firebase Console
  authDomain: 'YOUR_PROJECT_ID.firebaseapp.com',
  databaseURL: 'https://YOUR_PROJECT_ID.firebaseio.com',
  storageBucket: 'YOUR_PROJECT_ID.appspot.com',
  measurementId: 'YOUR_MEASUREMENT_ID',
);

// For Android:
static const FirebaseOptions android = FirebaseOptions(
  apiKey: 'YOUR_ANDROID_API_KEY',
  appId: 'YOUR_ANDROID_APP_ID',
  messagingSenderId: 'YOUR_SENDER_ID',
  projectId: 'YOUR_PROJECT_ID',
  storageBucket: 'YOUR_PROJECT_ID.appspot.com',
);
```

## Step 6: Install Dependencies

Run these commands in your project directory:

```bash
flutter pub get
flutter pub upgrade
```

## Step 7: Configure Android

### Update android/build.gradle:

```gradle
buildscript {
    ext.kotlin_version = '1.9.10'
    repositories {
        google()
        mavenCentral()
    }
    dependencies {
        classpath 'com.google.gms:google-services:4.4.0'
    }
}
```

### Update android/app/build.gradle:

Add at the end of the file:
```gradle
apply plugin: 'com.google.gms.google-services'
```

## Step 8: Configure iOS

1. Open iOS project:
```bash
open ios/Runner.xcworkspace
```

2. Select **Runner** project
3. Go to **Build Settings**
4. Search for "Swift Language Version"
5. Set to Swift 5.0 or higher

## Step 9: Test the App

```bash
# For Android
flutter run

# For iOS
flutter run -d ios

# For Web
flutter run -d web
```

## Firestore Database Structure

The app uses the following Firestore structure:

```
users/
  └── {uid}
      ├── uid: string
      ├── email: string
      ├── displayName: string
      └── createdAt: timestamp

chatRooms/
  └── {chatRoomId}
      ├── members: array[userId1, userId2]
      ├── memberNames: array[name1, name2]
      ├── lastMessage: string
      ├── lastMessageTime: timestamp
      ├── lastMessageSender: string
      ├── createdAt: timestamp
      └── messages/
          └── {messageId}
              ├── message: string
              ├── senderId: string
              ├── senderName: string
              └── timestamp: timestamp
```

## Features Implemented

✅ User Authentication (Sign Up / Login)
✅ User Listing
✅ Real-time Chat Messaging
✅ Chat Room Management
✅ Message History
✅ User Presence (via auth state)

## Troubleshooting

### Firebase Not Initializing
- Ensure `firebase_options.dart` has correct credentials
- Check that `google-services.json` is in `android/app/`
- For iOS, verify `GoogleService-Info.plist` is in Xcode

### Authentication Errors
- Enable Email/Password in Firebase Console
- Check Firestore Security Rules are published
- Verify user collection exists in Firestore

### Real-time Messages Not Updating
- Verify Cloud Firestore is enabled
- Check Security Rules allow read/write access
- Ensure user is authenticated

### Build Errors
- Run `flutter clean`
- Run `flutter pub get`
- Run `flutter pub upgrade`

## Next Steps (Optional)

1. **Add Image Support**: Integrate Firebase Storage
2. **Push Notifications**: Add Firebase Cloud Messaging
3. **User Search**: Implement search functionality
4. **Message Deletion**: Add delete message feature
5. **Typing Indicators**: Show when user is typing
6. **User Status**: Add online/offline status
7. **Message Encryption**: Add end-to-end encryption

## Resources

- [Firebase Documentation](https://firebase.google.com/docs)
- [FlutterFire Documentation](https://firebase.flutter.dev/)
- [Firebase Security Rules](https://firebase.google.com/docs/firestore/security/start)
- [Firebase Authentication](https://firebase.google.com/docs/auth)

