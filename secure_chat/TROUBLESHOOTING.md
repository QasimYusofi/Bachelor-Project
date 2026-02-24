# White Blank Screen Troubleshooting Guide

## Issue Description
App shows white blank screen on Android startup with "cancelDraw null isViewVisible: true" messages.

## Fixes Applied

### 1. **Enhanced Error Handling in main.dart**
- Added global Flutter error handler with `FlutterError.onError`
- Added try-catch for Firebase initialization
- Added detailed logging in AuthWrapper to track state changes

### 2. **Improved Serial Service Initialization**
- Added error handling to stream listeners in `setupListeners()`
- Added onError callbacks to prevent silent failures
- Wrapped entire listener setup in try-catch

### 3. **Better Error Display**
- AuthWrapper now shows error messages instead of blank screen
- Added error UI with retry button
- HomeScreen displays Firestore errors clearly

### 4. **Debug Logging**
Added console logging at critical points:
- Firebase initialization
- AuthWrapper state transitions
- User login status
- HomeScreen initialization

## Troubleshooting Steps

### Step 1: Check Console Output
Run the app and look for these log messages:
```
Firebase initialized successfully
AuthWrapper State: ...
User logged in: [uid]
```

If you see errors before these, that's the issue.

### Step 2: Common Issues & Solutions

**Issue: Firebase not initializing**
- Check `lib/firebase_options.dart` - ensure Android config is complete
- Check `android/app/google-services.json` is present
- Verify Firebase project ID matches your config

**Issue: Auth stream not emitting**
- Check Firebase authentication is enabled in console
- Verify Firestore security rules allow initial connection
- Check if user exists in Firebase Auth

**Issue: Serial service crashes**
- The app doesn't need USB serial at startup
- Serial listeners now have error handlers
- If serial plugin fails, app continues normally

### Step 3: Run with Verbose Logging
```bash
flutter run -v
```

Look for exceptions in the output. The enhanced error handlers will now catch and print them.

### Step 4: Check Specific Error Points

If you still see blank screen, the error is likely from:
1. **Firebase initialization** - check google-services.json
2. **Auth state stream** - check Firestore security rules
3. **HomeScreen initialization** - check Firestore connection

The logging will help identify which one.

## Next Steps

1. Run the app and share the console output
2. Look for any red error messages
3. Check if the app reaches "User logged in" or "No user logged in" logs
4. Share these logs so we can diagnose the exact issue

