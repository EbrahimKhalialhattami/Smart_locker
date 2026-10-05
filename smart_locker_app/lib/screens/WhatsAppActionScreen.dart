import 'dart:math';
import 'package:flutter/material.dart';
import 'package:url_launcher/url_launcher.dart';
import '../services/firebase_service.dart';

class WhatsAppActionScreen extends StatefulWidget {
  final String lockerId;

  const WhatsAppActionScreen({Key? key, required this.lockerId}) : super(key: key);

  @override
  _WhatsAppActionScreenState createState() => _WhatsAppActionScreenState();
}

class _WhatsAppActionScreenState extends State<WhatsAppActionScreen> {
  final Color _bgColor = const Color(0xFF0F172A);
  final Color _accentColor = const Color(0xFF00E5FF);

  String _generatedOtp = '';
  bool _isLoading = false;
  String _customerPhone = '';
  String _customerName = '';

  @override
  void initState() {
    super.initState();
    _generateOtp();
    _fetchCustomerData();
  }

  void _generateOtp() {

    final random = Random();
    int otp = 1000 + random.nextInt(9000);
    setState(() {
      _generatedOtp = otp.toString();
    });
  }

  Future<void> _fetchCustomerData() async {

    FirebaseService().getSingleLockerStream(widget.lockerId).first.then((locker) {
      setState(() {
        _customerPhone = locker.customerPhone ?? '';
        _customerName = locker.customerName ?? '';
      });
    });
  }

  Future<void> _sendWhatsAppAndComplete() async {
    if (_customerPhone.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('رقم هاتف العميل غير متوفر'), backgroundColor: Colors.red),
      );
      return;
    }

    setState(() => _isLoading = true);

    try {

      await FirebaseService().saveOtpAndCompleteDeposit(
        lockerId: widget.lockerId,
        otpCode: _generatedOtp,
      );


      final message = Uri.encodeComponent(
          'مرحباً العميل الكريم ${_customerName}،\n'
              'تم إيداع أمانتك بنجاح في الخزانة رقم ${widget.lockerId}.\n'
              'رمز فتح الخزانة الخاص بك هو: ${_generatedOtp}\n'
              '⏳ تنويه هام: هذا الرمز صالح لمدة 48 ساعة فقط، وسيتم إلغاء العملية تلقائياً بعد انقضاء هذه المدة.\n'
              'يرجى عدم مشاركة الرمز مع أي شخص.'
      );

      final whatsappUrl = Uri.parse('https://wa.me/$_customerPhone?text=$message');

      if (await canLaunchUrl(whatsappUrl)) {
        await launchUrl(whatsappUrl, mode: LaunchMode.externalApplication);
      } else {
        throw 'تعذر فتح تطبيق الواتساب';
      }

      if (mounted) {

        Navigator.of(context).popUntil((route) => route.isFirst);
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('تم إتمام دورة الإيداع وتحديث النظام بنجاح!'), backgroundColor: Colors.green),
        );
      }
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('حدث خطأ: $e'), backgroundColor: Colors.red),
      );
    } finally {
      if (mounted) setState(() => _isLoading = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: _bgColor,
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        title: Text('تأكيد رمز الأمانة - خزانة ${widget.lockerId}',
            style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold)),
        iconTheme: const IconThemeData(color: Colors.white),
      ),
      body: Padding(
        padding: const EdgeInsets.all(24.0),
        child: Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(Icons.message, size: 80, color: Colors.greenAccent),
              const SizedBox(height: 20),
              const Text(
                'تم الفتح الفيزيائي بنجاح، يجدر الآن إرسال الرمز للعميل',
                textAlign: TextAlign.center,
                style: TextStyle(color: Colors.white70, fontSize: 16),
              ),
              const SizedBox(height: 30),
              Container(
                padding: const EdgeInsets.all(24),
                decoration: BoxDecoration(
                  color: Colors.white.withOpacity(0.05),
                  borderRadius: BorderRadius.circular(20),
                  border: Border.all(color: Colors.white.withOpacity(0.1)),
                ),
                child: Column(
                  children: [
                    const Text('الرمز السري المولّد (OTP)', style: TextStyle(color: Colors.white54, fontSize: 14)),
                    const SizedBox(height: 12),
                    Text(
                      _generatedOtp,
                      style: TextStyle(color: _accentColor, fontSize: 40, fontWeight: FontWeight.bold, letterSpacing: 6),
                    ),
                  ],
                ),
              ),
              const SizedBox(height: 40),
              SizedBox(
                width: double.infinity,
                height: 55,
                child: ElevatedButton(
                  style: ElevatedButton.styleFrom(
                    backgroundColor: Colors.green,
                    foregroundColor: Colors.white,
                    shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
                    elevation: 8,
                  ),
                  onPressed: _isLoading ? null : _sendWhatsAppAndComplete,
                  child: _isLoading
                      ? const CircularProgressIndicator(color: Colors.white)
                      : const Text('إرسال الرمز عبر واتساب وإتمام العملية', style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}