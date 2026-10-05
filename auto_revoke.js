const admin = require("firebase-admin");

// قراءة بيانات الاعتماد والرابط من خوادم جيت هب المشفرة
const serviceAccount = JSON.parse(process.env.FIREBASE_SERVICE_ACCOUNT);
const databaseURL = process.env.FIREBASE_DATABASE_URL;

admin.initializeApp({
  credential: admin.credential.cert(serviceAccount),
  databaseURL: databaseURL
});

const db = admin.database();
const lockersRef = db.ref("lockers");

async function checkExpiredLockers() {
  try {
    const snapshot = await lockersRef.once("value");
    const lockers = snapshot.val();
    
    if (!lockers) {
      console.log("No lockers found.");
      process.exit(0);
    }

    const now = Date.now();
    const FORTY_EIGHT_HOURS_IN_MS = 48 * 60 * 60 * 1000;
    let updates = {};

    for (const lockerId in lockers) {
      const locker = lockers[lockerId];
      // التحقق إذا كانت الخزانة محجوزة وتمتلك طابعاً زمنياً
      if (locker.status !== "available" && locker.depositTimestamp) {
        const elapsed = now - locker.depositTimestamp;
        
        // إذا تجاوز الوقت 48 ساعة
        if (elapsed >= FORTY_EIGHT_HOURS_IN_MS) {
          updates[`${lockerId}/command`] = "revoke_otp";
          console.log(`Locker ${lockerId} expired! Revoke command sent.`);
        }
      }
    }

    if (Object.keys(updates).length > 0) {
      await lockersRef.update(updates);
      console.log("Database updated successfully.");
    } else {
      console.log("No expired lockers found at this time.");
    }
  } catch (error) {
    console.error("Error checking lockers:", error);
    process.exit(1);
  }
  process.exit(0); // إنهاء العملية بنجاح
}

checkExpiredLockers();
