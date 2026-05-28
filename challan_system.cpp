#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <windows.h>
using namespace std;

const string FILENAME = "challans.csv";

bool verifyHardwareKey() {
    // Establish connection handle to the physical target port
    // Ensure this matches your verified hardware configuration
    HANDLE hSerial = CreateFileA("\\\\.\\COM3", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hSerial == INVALID_HANDLE_VALUE) {
        return false; // Physical hardware dongle unmapped or disconnected
    }

    // Configure standard parameters to match runtime controller parameters
    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    GetCommState(hSerial, &dcbSerialParams);
    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    SetCommState(hSerial, &dcbSerialParams);

    // Transmit polling challenge byte down the TX line
    DWORD bytesWritten;
    char request = 'R';
    WriteFile(hSerial, &request, 1, &bytesWritten, NULL);

    // Buffer window allowing device execution loops to process and return strings
    Sleep(200);

    // Capture incoming hardware responses from the RX register
    char szBuff[11] = { 0 }; 
    DWORD bytesRead;
    bool isAuthorized = false;

    if (ReadFile(hSerial, szBuff, 10, &bytesRead, NULL)) {
        string receivedKey(szBuff);
        // Verify key integrity against baseline controller string
        if (receivedKey == "ACCESS_123") {
            isAuthorized = true;
        }
    }

    CloseHandle(hSerial); // Terminate interface allocations cleanly
    return isAuthorized;
}
// ================================================
// FUNCTION 1: initializeDatabase()
// Checks for CSV file, creates it if missing
// ================================================
void initializeDatabase() {
    ifstream checkFile(FILENAME.c_str());
    if (!checkFile) {
        ofstream newFile(FILENAME.c_str());
        newFile << "ChallanID,PlateID,OwnerName,Violation,FineAmt,Date,Status\n";
        newFile.close();
        cout << "[SYSTEM] Database created: challans.csv" << endl;
    } else {
        cout << "[SYSTEM] Database found: challans.csv" << endl;
    }
    checkFile.close();
}

// ================================================
// FUNCTION 2: isUnique(string id)
// Scans CSV to prevent duplicate Plate IDs
// ================================================
bool isUnique(string plateID) {
    ifstream file(FILENAME.c_str());
    string line;
    getline(file, line); // skip header
    while (getline(file, line)) {
        stringstream ss(line);
        string challanID, pid;
        getline(ss, challanID, ',');
        getline(ss, pid, ',');
        if (pid == plateID) {
            file.close();
            return false; // duplicate found
        }
    }
    file.close();
    return true; // unique
}

// ================================================
// FUNCTION 3: appendRecord(string data)
// Adds a new comma-separated record to CSV
// ================================================
void appendRecord(string data) {
    ofstream file(FILENAME.c_str(), ios::app);
    file << data << "\n";
    file.close();
    cout << "[SUCCESS] Challan saved to database.\n";
}

// ================================================
// FUNCTION 4: searchByID(string id)
// Sequential search - finds record by Plate ID
// Also shows owner name and updated status
// ================================================
void searchByID(string plateID) {
    ifstream file(FILENAME.c_str());
    string line;
    bool found = false;
    getline(file, line); // skip header
    while (getline(file, line)) {
        stringstream ss(line);
        string fields[7];
        for (int i = 0; i < 7; i++) getline(ss, fields[i], ',');
        // fields[1] is always PlateID
        if (fields[1] == plateID) {
            cout << "\n=============================\n";
            cout << "     CHALLAN RECORD FOUND    \n";
            cout << "=============================\n";
            cout << "Challan ID : " << fields[0] << endl;
            cout << "Plate ID   : " << fields[1] << endl;
            cout << "Owner Name : " << fields[2] << endl;  // owner always shown
            cout << "Violation  : " << fields[3] << endl;
            cout << "Fine (PKR) : " << fields[4] << endl;
            cout << "Date       : " << fields[5] << endl;
            cout << "Status     : " << fields[6] << endl;  // always latest status
            cout << "=============================\n";
            found = true;
        }
    }
    if (!found) cout << "[NOT FOUND] No challan found for Plate ID: " << plateID << "\n";
    file.close();
}

// ================================================
// FUNCTION 5: updateRecord(string id, string newData)
// Modifies or deletes entry using a temp file
// to maintain data integrity
// ================================================
void updateRecord(string plateID, string newData) {
    ifstream file(FILENAME.c_str());
    ofstream temp("temp.csv");
    string line;
    bool found = false;

    // write header first
    getline(file, line);
    temp << line << "\n";

    while (getline(file, line)) {
        stringstream ss(line);
        string fields[7];
        for (int i = 0; i < 7; i++) getline(ss, fields[i], ',');

        if (fields[1] == plateID) {
            found = true;
            if (newData != "DELETE") {
                temp << newData << "\n"; // write updated record
            }
            // if DELETE just skip — removes the record
        } else {
            temp << line << "\n"; // keep all other records
        }
    }
    file.close();
    temp.close();

    // replace original with updated temp file
    remove(FILENAME.c_str());
    rename("temp.csv", FILENAME.c_str());

    if (found) cout << "[SUCCESS] Record updated successfully.\n";
    else cout << "[NOT FOUND] No record with Plate ID: " << plateID << "\n";
}

// ================================================
// MAIN PROGRAM
// ================================================
int main() {
    cout << "==================================================\n";
    cout << "        CHALLAN SYSTEM\n";
    cout << "==================================================\n";
    cout << "Interrogating physical COM channels for hardware verification key...\n";

    // Gatekeeper enforcement validation
    if (!verifyHardwareKey()) {
        cout << "\n[ACCESS DENIED]\n";
        cout << "Ledger processing locked.\n";
        cout << "Insert certified ESP32\n";
        return -1; 
    }

    cout << "\n>> INTERLOCK VALIDATED. Ledger operations activated.\n";
    // Step 1: initialize database on startup
    initializeDatabase();

    cout << "\n========================================\n";
    cout << "   Traffic Challan Management System\n";
    cout << "   Team: Homo Compilicus\n";
    cout << "   ESP32-PC Hybrid Project\n";
    cout << "========================================\n";

    int choice;
    do {
        // Plate ID asked fresh every loop (simulates ESP32 sending ID)
        string plateID;
        cout << "\n[ESP32] Enter Plate ID (e.g. LHR-4521): ";
        cin >> plateID;
        cout << "[RECEIVED] Plate ID: " << plateID << "\n";

        cout << "\n--- MAIN MENU ---\n";
        cout << "1. Search challan\n";
        cout << "2. Add new challan\n";
        cout << "3. Update challan status\n";
        cout << "4. Delete challan\n";
        cout << "5. Exit\n";
        cout << "Enter choice: ";
        cin >> choice;

        // --- OPTION 1: Search ---
        if (choice == 1) {
            searchByID(plateID);

        // --- OPTION 2: Add new challan ---
        } else if (choice == 2) {
            if (!isUnique(plateID)) {
                cout << "[ERROR] Challan already exists for Plate ID: " << plateID << "\n";
            } else {
                string owner, violation, fine, date, challanID;
                cin.ignore();
                cout << "Challan ID : "; getline(cin, challanID);
                cout << "Owner Name : "; getline(cin, owner);
                cout << "Violation  : "; getline(cin, violation);
                cout << "Fine (PKR) : "; getline(cin, fine);
                cout << "Date       : "; getline(cin, date);
                string record = challanID+","+plateID+","+owner+","+violation+","+fine+","+date+",Unpaid";
                appendRecord(record);
                // auto confirm after adding
                cout << "\n[CONFIRM] Verifying saved record...\n";
                searchByID(plateID);
            }

        // ---- OPTION 3: Update status ---
        } else if (choice == 3) {
            ifstream file(FILENAME.c_str());
            string line;
            getline(file, line); // skip header
            bool found = false;
            while (getline(file, line)) {
                stringstream ss(line);
                string f[7];
                for (int i = 0; i < 7; i++) getline(ss, f[i], ',');
                if (f[1] == plateID) {
                    file.close();
                    found = true;
                    string newStatus;
                    cin.ignore();
                    cout << "Owner      : " << f[2] << endl;
                    cout << "Current Status: " << f[6] << endl;
                    cout << "Enter new status (Paid/Unpaid): ";
                    getline(cin, newStatus);
                    // rebuild full updated record with new status
                    string updated = f[0]+","+f[1]+","+f[2]+","+f[3]+","+f[4]+","+f[5]+","+newStatus;
                    updateRecord(plateID, updated);
                    // auto search after update so user sees new status
                    cout << "\n[CONFIRM] Updated record:\n";
                    searchByID(plateID);
                    break;
                }
            }
            if (!found) {
                file.close();
                cout << "[NOT FOUND] No challan found for Plate ID: " << plateID << "\n";
            }

        // --- OPTION 4: Delete challan ---
        } else if (choice == 4) {
            // show record first before deleting
            searchByID(plateID);
            char confirm;
            cout << "Are you sure you want to DELETE this record? (y/n): ";
            cin >> confirm;
            if (confirm == 'y' || confirm == 'Y') {
                updateRecord(plateID, "DELETE");
            } else {
                cout << "[CANCELLED] Record not deleted.\n";
            }

        // --- OPTION 5: Exit ---
        } else if (choice == 5) {
            cout << "\nExiting system. Goodbye!\n";
            cout << "             - Homo Compilicus\n";

        } else {
            cout << "[ERROR] Invalid choice. Try again.\n";
        }

    } while (choice != 5);

    system("pause");
    return 0;
}
