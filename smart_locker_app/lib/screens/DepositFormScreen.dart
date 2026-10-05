import 'dart:ui';
import 'package:flutter/material.dart';
import 'rfid_countdown_screen.dart';
import '../services/firebase_service.dart';

class DepositFormScreen extends StatefulWidget {
  final String lockerId;

  const DepositFormScreen({Key? key, required this.lockerId}) : super(key: key);

  @override
  _DepositFormScreenState createState() => _DepositFormScreenState();
}

class _DepositFormScreenState extends State<DepositFormScreen> {
  final _formKey = GlobalKey<FormState>();
  final TextEditingController _nameController = TextEditingController();
  final TextEditingController _phoneController = TextEditingController();
  bool _isLoading = false;

  // (Dark Theme Colors)
  final Color _bgColor = const Color(0xFF0F172A);
  final Color _glassColor = Colors.white.withOpacity(0.05);
  final Color _glassBorder = Colors.white.withOpacity(0.1);
  final Color _accentColor = const Color(0xFF00E5FF);

  void _submitForm() async {
    if (_formKey.currentState!.validate()) {
      setState(() => _isLoading = true);

      try {

        await FirebaseService().initiateDeposit(
          lockerId: widget.lockerId,
          customerName: _nameController.text,
          customerPhone: _phoneController.text,
        );




        if (mounted) {
          Navigator.pushReplacement(
              context,
              MaterialPageRoute(builder: (_) => RfidCountdownScreen(lockerId: widget.lockerId))
          );
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('تم إرسال الأمر للشريحة، بانتظار البطاقة...')),
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
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: _bgColor,
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        title: Text('إيداع أمانة - خزانة ${widget.lockerId}',
            style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold)),
        iconTheme: const IconThemeData(color: Colors.white),
      ),
      body: Stack(
        children: [
          // (Glow)
          Positioned(
            top: 50,
            left: -50,
            child: Container(
              width: 200,
              height: 200,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: _accentColor.withOpacity(0.15),
              ),
              child: BackdropFilter(
                filter: ImageFilter.blur(sigmaX: 50, sigmaY: 50),
                child: Container(color: Colors.transparent),
              ),
            ),
          ),

          Center(
            child: SingleChildScrollView(
              padding: const EdgeInsets.all(24.0),
              child: ClipRRect(
                borderRadius: BorderRadius.circular(24),
                child: BackdropFilter(
                  filter: ImageFilter.blur(sigmaX: 15, sigmaY: 15),
                  child: Container(
                    padding: const EdgeInsets.all(32.0),
                    decoration: BoxDecoration(
                      color: _glassColor,
                      borderRadius: BorderRadius.circular(24),
                      border: Border.all(color: _glassBorder, width: 1.5),
                    ),
                    child: Form(
                      key: _formKey,
                      child: Column(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          const Icon(Icons.security, size: 48, color: Colors.white70),
                          const SizedBox(height: 24),


                          _buildTextField(
                            controller: _nameController,
                            label: 'اسم العميل',
                            icon: Icons.person_outline,
                            validator: (value) => value!.isEmpty ? 'الرجاء إدخال اسم العميل' : null,
                          ),
                          const SizedBox(height: 16),

                          // حقل رقم الهاتف
                          _buildTextField(
                            controller: _phoneController,
                            label: 'رقم الهاتف (للواتساب)',
                            icon: Icons.phone_android,
                            keyboardType: TextInputType.phone,
                            validator: (value) => value!.isEmpty ? 'الرجاء إدخال رقم الهاتف' : null,
                          ),
                          const SizedBox(height: 32),


                          SizedBox(
                            width: double.infinity,
                            height: 55,
                            child: ElevatedButton(
                              style: ElevatedButton.styleFrom(
                                backgroundColor: _accentColor,
                                foregroundColor: _bgColor,
                                shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
                                elevation: 10,
                                shadowColor: _accentColor.withOpacity(0.5),
                              ),
                              onPressed: _isLoading ? null : _submitForm,
                              child: _isLoading
                                  ? const CircularProgressIndicator(color: Colors.black87)
                                  : const Text('تأكيد وبدء مهلة الفتح (30ث)',
                                  style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
                            ),
                          ),
                        ],
                      ),
                    ),
                  ),
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }


  Widget _buildTextField({
    required TextEditingController controller,
    required String label,
    required IconData icon,
    TextInputType keyboardType = TextInputType.text,
    String? Function(String?)? validator,
  }) {
    return TextFormField(
      controller: controller,
      keyboardType: keyboardType,
      validator: validator,
      style: const TextStyle(color: Colors.white),
      decoration: InputDecoration(
        labelText: label,
        labelStyle: const TextStyle(color: Colors.white54),
        prefixIcon: Icon(icon, color: _accentColor),
        filled: true,
        fillColor: Colors.black.withOpacity(0.2),
        border: OutlineInputBorder(
          borderRadius: BorderRadius.circular(16),
          borderSide: BorderSide.none,
        ),
        focusedBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(16),
          borderSide: BorderSide(color: _accentColor, width: 1),
        ),
      ),
    );
  }
}