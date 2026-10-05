class LockerModel {

  final String id;
  final String status;
  final String currentOtp;

  final String? customerName;
  final String? customerPhone;

  final int? depositTimestamp;

  LockerModel({
    required this.id,
    required this.status,
    required this.currentOtp,
    this.customerName,
    this.customerPhone,
    this.depositTimestamp,
  });

  factory LockerModel.fromMap(Map<String, dynamic> map, String documentId) {
    return LockerModel(
      id: documentId,
      // available, pending_rfid, opened_by_admin, occupied
      status: map['status'] ?? 'available',
      currentOtp: map['currentOtp'] ?? '',
      customerName: map['customerName'],
      customerPhone: map['customerPhone'],
      depositTimestamp: map['depositTimestamp'],
    );
  }

  Map<String, dynamic> toMap() {
    return {
      'status': status,
      'currentOtp': currentOtp,
      'customerName': customerName,
      'customerPhone': customerPhone,
      'depositTimestamp': depositTimestamp,
    };
  }
}