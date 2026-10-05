import 'dart:async';
import 'package:flutter/material.dart';
import '../models/locker_model.dart';
import '../services/firebase_service.dart';
import 'WhatsAppActionScreen.dart';
class RfidCountdownScreen extends StatefulWidget {
  final String lockerId;

  const RfidCountdownScreen({Key? key, required this.lockerId}) : super(key: key);

  @override
  _RfidCountdownScreenState createState() => _RfidCountdownScreenState();
}

class _RfidCountdownScreenState extends State<RfidCountdownScreen> {
  int _timeLeft = 30;
  Timer? _timer;
  late StreamSubscription<LockerModel> _lockerSubscription;
  bool _isProcessing = false;

  final Color _bgColor = const Color(0xFF0F172A);
  final Color _accentColor = const Color(0xFF00E5FF);

  @override
  void initState() {
    super.initState();
    _startCountdownAndListen();
  }

  void _startCountdownAndListen() {

    _timer = Timer.periodic(const Duration(seconds: 1), (timer) {
      if (_timeLeft > 0) {
        setState(() => _timeLeft--);
      } else {
        _handleTimeout();
      }
    });


    _lockerSubscription = FirebaseService().getSingleLockerStream(widget.lockerId).listen((locker) {

      if (locker.status == 'opened_by_admin' && !_isProcessing) {
        _handleSuccess();
      }
    });
  }

  void _handleSuccess() {
    _isProcessing = true;
    _timer?.cancel();
    _lockerSubscription.cancel();



    Navigator.pushReplacement(
      context,
      MaterialPageRoute(builder: (_) => WhatsAppActionScreen(lockerId: widget.lockerId)),
    );



    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('تم الفتح بنجاح! جاري الانتقال لخطوة الواتساب...'), backgroundColor: Colors.green),
    );
  }

  Future<void> _handleTimeout() async {
    _isProcessing = true;
    _timer?.cancel();
    _lockerSubscription.cancel();


    await FirebaseService().cancelDeposit(widget.lockerId);

    if (mounted) {
      Navigator.pop(context);
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('انتهت المهلة ولم يتم تمرير البطاقة. تم إلغاء الطلب.'),
          backgroundColor: Colors.red,
        ),
      );
    }
  }

  @override
  void dispose() {
    _timer?.cancel();
    _lockerSubscription.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: _bgColor,
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const Icon(Icons.nfc, size: 80, color: Colors.white70),
            const SizedBox(height: 30),
            const Text(
              'يرجى تمرير بطاقة المشرف لفتح الخزانة',
              style: TextStyle(color: Colors.white, fontSize: 18, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 40),

            Stack(
              alignment: Alignment.center,
              children: [
                SizedBox(
                  width: 120,
                  height: 120,
                  child: CircularProgressIndicator(
                    value: _timeLeft / 30, //
                    strokeWidth: 8,
                    color: _timeLeft > 10 ? _accentColor : Colors.red,
                    backgroundColor: Colors.white12,
                  ),
                ),
                Text(
                  '$_timeLeft',
                  style: TextStyle(
                    color: _timeLeft > 10 ? Colors.white : Colors.red,
                    fontSize: 40,
                    fontWeight: FontWeight.bold,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 40),
            TextButton(
              onPressed: _handleTimeout,
              child: const Text('إلغاء العملية فوراً', style: TextStyle(color: Colors.redAccent, fontSize: 16)),
            ),
          ],
        ),
      ),
    );
  }
}