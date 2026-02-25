import 'package:flutter/material.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:secure_chat/firebase_options.dart';
import 'package:secure_chat/services/firebase_service.dart';
import 'package:secure_chat/screens/login_screen.dart';
import 'package:secure_chat/screens/signup_screen.dart';
import 'package:secure_chat/screens/home_screen.dart';
import 'package:secure_chat/screens/users_screen.dart';
import 'package:secure_chat/screens/chat_screen.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  // Set up error handling
  FlutterError.onError = (FlutterErrorDetails details) {
    print('Flutter Error: ${details.exception}');
    print('Stack trace: ${details.stack}');
  };

  try {
    // Check if Firebase is already initialized
    if (Firebase.apps.isEmpty) {
      await Firebase.initializeApp(
        options: DefaultFirebaseOptions.currentPlatform,
      );
      print('Firebase initialized successfully');
    } else {
      print('Firebase already initialized');
    }
  } catch (e) {
    print('Firebase initialization error: $e');
  }
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Secure Chat',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.blue),
        useMaterial3: true,
      ),
      home: const AuthWrapper(),
      routes: {
        '/login': (context) => const LoginScreen(),
        '/signup': (context) => const SignupScreen(),
        '/home': (context) => const HomeScreen(),
        '/users': (context) => const UsersScreen(),
        '/chat': (context) {
          final args = ModalRoute.of(context)?.settings.arguments;

          // Handle null arguments (when refreshing directly on chat screen)
          if (args == null || args is! Map<String, dynamic>) {
            return Scaffold(
              appBar: AppBar(title: const Text('Error')),
              body: Center(
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    const Text('Invalid chat parameters'),
                    const SizedBox(height: 16),
                    ElevatedButton(
                      onPressed: () => Navigator.of(context).pushReplacementNamed('/home'),
                      child: const Text('Go to Home'),
                    ),
                  ],
                ),
              ),
            );
          }

          return ChatScreen(
            chatRoomId: args['chatRoomId'] ?? '',
            otherUserId: args['otherUserId'] ?? '',
            otherUserName: args['otherUserName'] ?? 'Unknown',
          );
        },
      },
    );
  }
}

class AuthWrapper extends StatelessWidget {
  const AuthWrapper({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    final firebaseService = FirebaseService();

    return StreamBuilder(
      stream: firebaseService.authStateChanges,
      builder: (context, snapshot) {
        print('AuthWrapper State: ${snapshot.connectionState}, HasData: ${snapshot.hasData}, HasError: ${snapshot.hasError}');

        if (snapshot.hasError) {
          print('AuthWrapper Error: ${snapshot.error}');
          return Scaffold(
            body: Center(
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  const Text('Authentication Error'),
                  const SizedBox(height: 16),
                  Text(snapshot.error.toString(), textAlign: TextAlign.center),
                  const SizedBox(height: 16),
                  ElevatedButton(
                    onPressed: () {
                      // Refresh by navigating
                    },
                    child: const Text('Retry'),
                  ),
                ],
              ),
            ),
          );
        }

        if (snapshot.connectionState == ConnectionState.waiting) {
          return const Scaffold(
            body: Center(child: CircularProgressIndicator()),
          );
        }

        if (snapshot.hasData) {
          print('User logged in: ${snapshot.data?.uid}');
          return const HomeScreen();
        }

        print('No user logged in, showing login screen');
        return const LoginScreen();
      },
    );
  }
}
