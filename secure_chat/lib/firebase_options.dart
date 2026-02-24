import 'package:firebase_core/firebase_core.dart' show FirebaseOptions;
import 'package:flutter/foundation.dart'
    show defaultTargetPlatform, kIsWeb, TargetPlatform;

class DefaultFirebaseOptions {
  static FirebaseOptions get currentPlatform {
    if (kIsWeb) {
      return web;
    }
    switch (defaultTargetPlatform) {
      case TargetPlatform.android:
        return android;
      case TargetPlatform.iOS:
        return ios;
      case TargetPlatform.macOS:
        return macos;
      case TargetPlatform.windows:
        throw UnsupportedError(
          'DefaultFirebaseOptions have not been configured for windows - '
          'you can reconfigure this by running the FlutterFire CLI again.',
        );
      case TargetPlatform.linux:
        throw UnsupportedError(
          'DefaultFirebaseOptions have not been configured for linux - '
          'you can reconfigure this by running the FlutterFire CLI again.',
        );
      default:
        throw UnsupportedError(
          'DefaultFirebaseOptions are not supported for this platform.',
        );
    }
  }

  static const FirebaseOptions web = FirebaseOptions(
    apiKey: "AIzaSyCzQI8Z5pYmMPFXikD7e3CFiD1p0p-HIWM",
    appId: "1:9317106466:web:1cbb758bb67085d7681b7b",
    messagingSenderId: "9317106466",
    projectId: "chat-2df0c",
    authDomain: "chat-2df0c.firebaseapp.com",
    databaseURL: 'https://chat-2df0c.firebaseio.com',
    storageBucket: "chat-2df0c.firebasestorage.app",
    measurementId: 'YOUR_MEASUREMENT_ID',
  );

  static const FirebaseOptions android = FirebaseOptions(
    apiKey: "AIzaSyCzQI8Z5pYmMPFXikD7e3CFiD1p0p-HIWM",
    appId: '1:9317106466:android:16eda24cddab41a6681b7b',
    messagingSenderId: "9317106466",
    projectId: "chat-2df0c",
    storageBucket: "chat-2df0c.firebasestorage.app",
  );

  static const FirebaseOptions ios = FirebaseOptions(
    apiKey: 'YOUR_IOS_API_KEY',
    appId: 'YOUR_IOS_APP_ID',
    messagingSenderId: 'YOUR_MESSAGING_SENDER_ID',
    projectId: 'YOUR_PROJECT_ID',
    storageBucket: 'YOUR_PROJECT_ID.appspot.com',
    iosBundleId: 'com.example.secureChat',
  );

  static const FirebaseOptions macos = FirebaseOptions(
    apiKey: 'YOUR_MACOS_API_KEY',
    appId: 'YOUR_MACOS_APP_ID',
    messagingSenderId: 'YOUR_MESSAGING_SENDER_ID',
    projectId: 'YOUR_PROJECT_ID',
    storageBucket: 'YOUR_PROJECT_ID.appspot.com',
    iosBundleId: 'com.example.secureChat',
  );
}
// // Import the functions you need from the SDKs you need
// import { initializeApp } from "firebase/app";
// // TODO: Add SDKs for Firebase products that you want to use
// // https://firebase.google.com/docs/web/setup#available-libraries
//
// // Your web app's Firebase configuration
// const firebaseConfig = {
//   apiKey: "AIzaSyCzQI8Z5pYmMPFXikD7e3CFiD1p0p-HIWM",
//   authDomain: "chat-2df0c.firebaseapp.com",
//   projectId: "chat-2df0c",
//   storageBucket: "chat-2df0c.firebasestorage.app",
//   messagingSenderId: "9317106466",
//   appId: "1:9317106466:web:1cbb758bb67085d7681b7b"
// };
//
// // Initialize Firebase
// const app = initializeApp(firebaseConfig);
