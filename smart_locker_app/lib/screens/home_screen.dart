import 'dart:ui';
import 'package:flutter/material.dart';
import '../models/locker_model.dart';
import '../services/firebase_service.dart';
import 'DepositFormScreen.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({Key? key}) : super(key: key);

  @override
  _HomeScreenState createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  final Color _bgColor = const Color(0xFF0F172A);

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: _bgColor,
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        title: const Text('لوحة تحكم الخزائن الذكية',
            style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold)),
        centerTitle: true,
      ),
      body: Stack(
        children: [
          Positioned(
            top: 100,
            right: -50,
            child: Container(
              width: 250,
              height: 250,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: Colors.blueAccent.withOpacity(0.1),
              ),
              child: BackdropFilter(
                filter: ImageFilter.blur(sigmaX: 80, sigmaY: 80),
                child: Container(color: Colors.transparent),
              ),
            ),
          ),


          StreamBuilder<List<LockerModel>>(
            stream: FirebaseService().getLockersStream(),
            builder: (context, snapshot) {
              if (snapshot.connectionState == ConnectionState.waiting) {
                return const Center(child: CircularProgressIndicator(color: Colors.cyanAccent));
              }

              if (snapshot.hasError) {
                return Center(child: Text('حدث خطأ في الاتصال: ${snapshot.error}', style: const TextStyle(color: Colors.red)));
              }

              if (!snapshot.hasData || snapshot.data!.isEmpty) {
                return const Center(child: Text('لا توجد بيانات للخزائن', style: TextStyle(color: Colors.white70)));
              }

              final lockers = snapshot.data!;


              return GridView.builder(
                padding: const EdgeInsets.all(24),
                gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                  crossAxisCount: 2,
                  crossAxisSpacing: 20,
                  mainAxisSpacing: 20,
                  childAspectRatio: 0.85,
                ),
                itemCount: lockers.length,
                itemBuilder: (context, index) {
                  return _buildLockerCard(lockers[index]);
                },
              );
            },
          ),
        ],
      ),
    );
  }


  Widget _buildLockerCard(LockerModel locker) {

    Color statusColor;
    String statusText;
    IconData statusIcon;

    switch (locker.status) {
      case 'available':
        statusColor = Colors.greenAccent;
        statusText = 'متاحة';
        statusIcon = Icons.check_circle_outline;
        break;
      case 'occupied':
        statusColor = Colors.redAccent;
        statusText = 'مشغولة';
        statusIcon = Icons.lock_outline;
        break;
      case 'pending_rfid':
        statusColor = Colors.orangeAccent;
        statusText = 'بانتظار البطاقة';
        statusIcon = Icons.nfc;
        break;
      default:
        statusColor = Colors.grey;
        statusText = 'غير معروف';
        statusIcon = Icons.help_outline;
    }

    return GestureDetector(
      onTap: () {

        if (locker.status == 'available') {
          Navigator.push(
            context,
            MaterialPageRoute(builder: (_) => DepositFormScreen(lockerId: locker.id)),
          );
        } else {

          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(content: Text('الخزانة حالياً: $statusText')),
          );
        }
      },
      child: ClipRRect(
        borderRadius: BorderRadius.circular(20),
        child: BackdropFilter(
          filter: ImageFilter.blur(sigmaX: 10, sigmaY: 10),
          child: Container(
            decoration: BoxDecoration(
              color: Colors.white.withOpacity(0.05),
              borderRadius: BorderRadius.circular(20),
              border: Border.all(color: statusColor.withOpacity(0.5), width: 1.5),
            ),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Text(
                  locker.id,
                  style: const TextStyle(color: Colors.white, fontSize: 40, fontWeight: FontWeight.bold),
                ),
                const SizedBox(height: 10),
                Icon(statusIcon, color: statusColor, size: 36),
                const SizedBox(height: 10),
                Text(
                  statusText,
                  style: TextStyle(color: statusColor, fontSize: 16, fontWeight: FontWeight.bold),
                ),
                if (locker.status == 'occupied') ...[
                  const SizedBox(height: 8),
                  Text(
                    locker.customerName ?? '',
                    style: const TextStyle(color: Colors.white70, fontSize: 12),
                    overflow: TextOverflow.ellipsis,
                  ),
                ]
              ],
            ),
          ),
        ),
      ),
    );
  }
}