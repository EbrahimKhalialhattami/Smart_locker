import 'package:firebase_database/firebase_database.dart';
import '../models/locker_model.dart';

class FirebaseService {

  static final FirebaseService _instance = FirebaseService._internal();

  factory FirebaseService() => _instance;

  FirebaseService._internal();


  final DatabaseReference _lockersRef = FirebaseDatabase.instance.ref(
      'lockers');

   Stream<List<LockerModel>> getLockersStream() {
     return _lockersRef.onValue.map((event) {
       if (event.snapshot.value == null) return [];
       final data = event.snapshot.value;
       final List<LockerModel> lockersList = [];

      if (data is Map) {
        data.forEach((key, value) {
          if (value != null && value is Map) {

            lockersList.add(LockerModel.fromMap(Map<String, dynamic>.from(value as Map), key.toString()));
          }
        });
      }
      else if (data is List) {
        for (int i = 0; i < data.length; i++) {
          if (data[i] != null && data[i] is Map) {

            lockersList.add(LockerModel.fromMap(Map<String, dynamic>.from(data[i] as Map), i.toString()));
          }
        }
      }

      return lockersList;
    });
  }


  Future<void> initiateDeposit({
    required String lockerId,
    required String customerName,
    required String customerPhone,
  }) async {
    try {
      final int currentTimestamp = DateTime
          .now()
          .millisecondsSinceEpoch;


      await _lockersRef.child(lockerId).update({
        'status': 'pending_rfid',
        'customerName': customerName,
        'customerPhone': customerPhone,
        'depositTimestamp': currentTimestamp,
      });
      await _lockersRef.child(lockerId).child('command').set('prepare_rfid');
    } catch (e) {
      throw Exception('فشل في إرسال أمر الإيداع: $e');
    }
  }

  Stream<LockerModel> getSingleLockerStream(String lockerId) {
    return _lockersRef
        .child(lockerId)
        .onValue
        .map((event) {
      if (event.snapshot.value != null) {
        return LockerModel.fromMap(
          Map<String, dynamic>.from(event.snapshot.value as Map),
          lockerId,
        );
      }
      throw Exception("بيانات الخزانة غير موجودة");
    });
  }

  Future<void> cancelDeposit(String lockerId) async {
    try {
      await _lockersRef.child(lockerId).update({
        'status': 'available',
        'customerName': null,
        'customerPhone': null,
        'depositTimestamp': null,
      });
      await _lockersRef.child(lockerId).child('command').set('none');
    } catch (e) {
      throw Exception('فشل في الإلغاء: $e');
    }
  }

  Future<void> saveOtpAndCompleteDeposit({
    required String lockerId,
    required String otpCode,
  }) async {
    try {
      await _lockersRef.child(lockerId).update({
        'status': 'occupied',
        'currentOtp': otpCode,
        'command': 'none',
      });
    } catch (e) {
      throw Exception('فشل في حفظ رمز OTP: $e');
    }
  }
}