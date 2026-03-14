#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <string>
using namespace std;

struct IndexEntry
{
    string id;
    long offset;
};

class SecondaryIndexDoctorID
{
private:
    string indexfile;
    string sourcefile;

    struct DoctorEntry
    {
        string doctorID;
        vector<string> appIDs;
        vector<long> offsets;
    };

    vector<DoctorEntry> indexList;

    int findDoctor(const string& key) const
    {
        for (size_t i = 0; i < indexList.size(); ++i)
            if (indexList[i].doctorID == key)
                return (int)i;
        return -1;
    }

public:
    SecondaryIndexDoctorID(const string& idxFile, const string& srcFile)
            : indexfile(idxFile), sourcefile(srcFile) {
    }

    void createIndex()
    {
        ifstream file(sourcefile, ios::binary);
        if (!file) {
            cout << "Error opening source file: " << sourcefile << endl;
            return;
        }

        indexList.clear();
        string line;
        long offset = 0;

        while (getline(file, line))
        {
            long currentOffset = file.tellg();
            if (currentOffset != -1)
                offset = currentOffset - line.length() - 1;
            else
                offset = 0;

            if (line.empty()) continue;

            stringstream ss(line);
            string prefix, appID, datePart, doctorID;

            getline(ss, prefix, '|');
            if (prefix.find("014") == string::npos) continue;

            ss >> ws;
            getline(ss, appID, '|');
            ss >> ws;
            getline(ss, datePart, '|');
            ss >> ws;
            getline(ss, doctorID);

            appID = appID.substr(0, 2);
            doctorID = doctorID.substr(0, 2);
            if (appID.length() == 1) appID = "0" + appID;
            if (doctorID.length() == 1) doctorID = "0" + doctorID;

            int pos = findDoctor(doctorID);
            if (pos == -1)
                indexList.push_back({ doctorID, {appID}, {offset} });
            else {
                indexList[pos].appIDs.push_back(appID);
                indexList[pos].offsets.push_back(offset);
            }
        }
        file.close();
        saveIndex();
        cout << "SecondaryIndexDoctorID created successfully!\n";
    }

    void saveIndex()
    {
        ofstream idx(indexfile, ios::trunc);
        sort(indexList.begin(), indexList.end(),
             [](const DoctorEntry& a, const DoctorEntry& b) { return a.doctorID < b.doctorID; });

        for (const auto& entry : indexList)
        {
            idx << entry.doctorID << "|";
            for (size_t i = 0; i < entry.offsets.size(); ++i)
            {
                idx << entry.offsets[i];
                if (i < entry.offsets.size() - 1) idx << ",";
            }
            idx << "\n";
        }
        idx.close();
    }

    void loadIndex()
    {
        ifstream idx(indexfile);
        if (!idx)
        {
            cout << "Index file missing! Run createIndex() first.\n";
            return;
        }
        indexList.clear();
        string line;
        while (getline(idx, line))
        {
            if (line.empty()) continue;
            stringstream ss(line);
            string docID, offsetStr;
            getline(ss, docID, '|');
            getline(ss, offsetStr);

            DoctorEntry entry{ docID };
            stringstream offSS(offsetStr);
            string token;
            while (getline(offSS, token, ','))
                if (!token.empty())
                    entry.offsets.push_back(stol(token));
            entry.appIDs.resize(entry.offsets.size());
            indexList.push_back(entry);
        }
        idx.close();
        cout << "Index loaded successfully!\n";
    }

    vector<pair<string, long>> searchByID(const string& keyID) const
    {
        vector<pair<string, long>> results;
        string searchKey = keyID;
        if (searchKey.length() == 1) searchKey = "0" + searchKey;

        int pos = findDoctor(searchKey);
        if (pos == -1) return results;

        ifstream data(sourcefile);
        if (!data) {
            cout << "Error: Cannot open appointments.txt!\n";
            return results;
        }

        for (long offset : indexList[pos].offsets)
        {
            data.seekg(offset);
            string line;
            if (getline(data, line))
            {
                stringstream ss(line);
                string temp, appID;
                getline(ss, temp, '|');
                getline(ss, appID, '|');
                appID = appID.substr(0, 2);
                if (appID.length() == 1) appID = "0" + appID;
                results.push_back({ appID, offset });
            }
        }
        data.close();
        return results;
    }

    string getRecordAtOffset(long offset) const
    {
        ifstream data(sourcefile);
        if (!data) return "ERROR: Cannot open appointments.txt";

        data.seekg(offset);
        string line;
        if (getline(data, line))
            return line;
        return "ERROR: Invalid offset";
    }
};

// ====================== Secondary Index on Doctor Name (doctors.txt) ======================
class SecondaryIndexDoctorName
{
private:
    string indexfile;
    string sourcefile;
    vector<IndexEntry> indexList;

    int binarySearch(const string& key) const
    {
        int low = 0, high = (int)indexList.size() - 1;
        while (low <= high)
        {
            int mid = (low + high) / 2;
            if (indexList[mid].id == key) return mid;
            if (indexList[mid].id < key) low = mid + 1;
            else high = mid - 1;
        }
        return -1;
    }

public:
    SecondaryIndexDoctorName(const string& idxFile, const string& srcFile)
            : indexfile(idxFile), sourcefile(srcFile) {
    }


  void createIndex()
    {
        ifstream file(sourcefile);
        if (!file) {
            cout << "Error opening doctors.txt!\n";
            return;
        }

        indexList.clear();

        string line;
        long currentOffset = 0;
        long counter = 0;

        while (getline(file, line))
        {
            size_t p1 = line.find('|');
            size_t p2 = line.find('|', p1 + 1);
            size_t p3 = line.find('|', p2 + 1);

            if (p1 != string::npos && p2 != string::npos && p3 != string::npos)
            {
                string name = line.substr(p2 + 1, p3 - p2 - 1);
                indexList.push_back({name, currentOffset});
            }

            // Move to next record: add the length of this line + newline character
            currentOffset += line.length() + 2;
        }

        file.close();
        saveIndex();
    }

    void saveIndex()
    {
        ofstream idx(indexfile, ios::trunc);
        sort(indexList.begin(), indexList.end(),
             [](const IndexEntry& a, const IndexEntry& b) { return a.id < b.id; });
        for (const auto& e : indexList)
            idx << e.id << "|" << e.offset << "\n";
        idx.close();
    }

    void loadIndex()
    {
        ifstream idx(indexfile);
        if (!idx)
        {
            cout << "Doctor name index missing! Run createIndex first.\n";
            return;
        }
        indexList.clear();
        string line;
        while (getline(idx, line))
        {
            stringstream ss(line);
            string name, off;
            getline(ss, name, '|');
            getline(ss, off);
            if (!off.empty())
                indexList.push_back({ name, stol(off) });
        }
        idx.close();
        cout << "Doctor name index loaded.\n";
    }

    // Updated function: returns both offset and full record
    pair<long, string> searchByName(const string& name) const
    {
        int pos = binarySearch(name);
        if (pos == -1)
            return { -1, "Doctor not found" };

        long offset = indexList[pos].offset;

        ifstream data(sourcefile);
        if (!data)
            return { -1, "ERROR: Cannot open doctors.txt" };

        data.seekg(offset);
        string record;
        if (getline(data, record))
        {
            data.close();
            return { offset, record };
        }

        data.close();

        return { offset, "ERROR: Failed to read record" };
    }

    long getOffsetByName(const string& name) const
    {
        int pos = binarySearch(name);
        return (pos == -1) ? -1 : indexList[pos].offset;
    }

    string getDoctorRecord(long offset) const
    {
        ifstream data(sourcefile);
        if (!data) return "ERROR: Cannot open doctors.txt";
        data.seekg(offset);
        string line;
        getline(data, line);
        return line.empty() ? "ERROR: Empty record" : line;
    }
};


class PrimaryIndex {
private:
    string indexfile;
    string sourcefile;

    int binarySearch(const string& key) {
        int low = 0, high = indexList.size() - 1;

        while (low <= high) {
            int mid = (low + high) / 2;

            if (indexList[mid].id == key)
                return mid;

            if (indexList[mid].id < key)
                low = mid + 1;
            else
                high = mid - 1;
        }
        return -1;
    }

public:

    PrimaryIndex(string idxFile, string srcFile)
            : indexfile(idxFile), sourcefile(srcFile) {
    }

    vector<IndexEntry> indexList;

    void loadIndex() {
        ifstream idx(indexfile);
        if (!idx) {
            cout << "Error: Index file missing!\n";
            return;
        }

        indexList.clear();
        string line;

        while (getline(idx, line)) {
            stringstream ss(line);
            string id, off;
            getline(ss, id, '|');
            getline(ss, off);

            indexList.push_back({ id, stol(off) });
        }
        idx.close();
    }


    void saveIndex() {
        ofstream idx(indexfile, ios::trunc);
        if (!idx) {
            cout << "Error writing to " << indexfile << "!\n";
            return;
        }

        // Sort to enable binary search
        sort(indexList.begin(), indexList.end(),
             [](const IndexEntry& a, const IndexEntry& b) {
                 return a.id < b.id;
             });

        for (const auto& entry : indexList) {
            idx << entry.id << "|" << entry.offset << "\n";
        }

        idx.close();
    }

    long indexByID(const string& keyID) {
        int pos = binarySearch(keyID);

        if (pos == -1)
            return -1; // Not found

        return indexList[pos].offset; // Just return the beginning of the record
    }

    long positionInVec(const string& keyID) {
        return binarySearch(keyID);
    }

    string readRecordAtOffset(long offset) {
        if (offset < 0)
            return "Record not found";

        ifstream data(sourcefile);
        if (!data)
            return "Source file missing!";

        data.seekg(offset);

        string record;
        getline(data, record);

        data.close();

        // Remove carriage return if present (Windows line endings)
        if (!record.empty() && record.back() == '\r') {
            record.pop_back();
        }

        return record;
    }

    void addToIndex(const string& id, long offset) {
        indexList.push_back({ id, offset });
    }

    // Sort the index vector by ID
    void sortIndex() {
        sort(indexList.begin(), indexList.end(),
             [](const IndexEntry& a, const IndexEntry& b) {
                 return a.id < b.id;
             });
    }

    // Add a new entry then sort immediately (recommended)
    void addAndSort(const string& id, long offset) {
        addToIndex(id, offset);
        sortIndex();
    }
};

class Insert
{
private:

    // -------------------------
    //  Utility Functions
    // -------------------------

    string formatLength(int len)
    {
        string s = to_string(len);
        if (s.length() < 3)
            s = string(3 - s.length(), '0') + s;
        return s;
    }

    string formatID(int id)
    {
        string s = to_string(id);
        if (s.length() == 1) s = "0" + s;
        return s;
    }

    int idStringToInt(const string& s)
    {
        try { return stoi(s); }
        catch (...) { return 0; }
    }

    string normalizeName(const string& s)
    {
        string temp = s;
        for (char& c : temp)
            c = tolower(c);

        int i = 0;
        while (i < temp.size() && temp[i] == ' ')
            i++;

        int j = temp.size() - 1;
        while (j >= 0 && temp[j] == ' ')
            j--;

        if (i > j) return "";

        string out = "";
        bool spaceFound = false;

        for (int k = i; k <= j; k++)
        {
            if (temp[k] == ' ')
            {
                if (!spaceFound)
                {
                    out += ' ';
                    spaceFound = true;
                }
            }
            else
            {
                out += temp[k];
                spaceFound = false;
            }
        }

        return out;
    }

    bool doctorNameExists(const string& newName)
    {
        ifstream file("doctors.txt");
        if (!file) return false;

        string line;
        string target = normalizeName(newName);

        while (getline(file, line))
        {
            if (line.size() < 5) continue;

            size_t p1 = line.find('|');
            size_t p2 = line.find('|', p1 + 1);
            size_t p3 = line.find('|', p2 + 1);
            if (p1 == string::npos || p2 == string::npos || p3 == string::npos)
                continue;

            if (line[3] == '*') continue; // deleted

            string existingName = line.substr(p2 + 1, p3 - p2 - 1);

            if (normalizeName(existingName) == target)
                return true;
        }
        return false;
    }

    bool doctorIDValid(const string& docID)
    {
        ifstream file("doctors.txt");
        if (!file) return false;

        string line;
        while (getline(file, line))
        {
            if (line.size() < 5) continue;
            if (line[3] == '*') continue;

            size_t p1 = line.find('|');
            size_t p2 = line.find('|', p1 + 1);
            if (p1 == string::npos || p2 == string::npos) continue;

            string existingID = line.substr(p1 + 1, p2 - p1 - 1);

            if (existingID == docID)
                return true;
        }
        return false;
    }

    struct Slot { long offset; int length; };

    vector<Slot> loadAvailList(const string& filename)
    {
        vector<Slot> v;
        ifstream file(filename);
        if (!file) return v;

        long off;
        int len;
        while (file >> off >> len)
            v.push_back({ off, len });

        return v;
    }

    void saveAvailList(const string& filename, const vector<Slot>& v)
    {
        ofstream file(filename, ios::trunc);
        for (auto& s : v)
            file << s.offset << " " << s.length << "\n";
    }

    bool findFirstFit(const vector<Slot>& v, int required,
                      int& idx, long& off, int& len)
    {
        for (int i = 0; i < v.size(); i++)
        {
            if (v[i].length >= required)
            {
                idx = i;
                off = v[i].offset;
                len = v[i].length;
                return true;
            }
        }
        return false;
    }

    string readOldIDAtOffset(const string& filename, long offset)
    {
        ifstream file(filename);
        if (!file) return "00";

        file.seekg(offset);
        string line;
        getline(file, line);

        size_t p1 = line.find('|');
        size_t p2 = line.find('|', p1 + 1);

        if (p1 == string::npos || p2 == string::npos) return "00";

        return line.substr(p1 + 1, p2 - p1 - 1);
    }

    string getLastIDFromFile(const string& filename)
    {
        ifstream file(filename);
        if (!file) return "00";

        string line, last;
        while (getline(file, line))
            if (!line.empty()) last = line;

        if (last.empty()) return "00";

        size_t p1 = last.find('|');
        size_t p2 = last.find('|', p1 + 1);

        if (p1 == string::npos || p2 == string::npos) return "00";

        return last.substr(p1 + 1, p2 - p1 - 1);
    }

    string buildDoctorRecord(const string& id, const string& name,
                             const string& address, int totalLen = -1)
    {
        string tail = " |" + id + "|" + name + "|" + address;
        int currentTotal = 3 + tail.length();

        if (totalLen != -1 && totalLen > currentTotal)
        {
            int pad = totalLen - currentTotal;
            tail += string(pad, ' ');
        }

        return formatLength(tail.length()) + tail;
    }

    string buildAppointmentRecord(const string& appID,
                                  const string& date, const string& doctorID,
                                  int totalLen = -1)
    {
        string tail = " |" + appID + "|" + date + "|" + doctorID;
        int currentTotal = 3 + tail.length();

        if (totalLen != -1 && totalLen > currentTotal)
        {
            int pad = totalLen - currentTotal;
            tail += string(pad, ' ');
        }

        return formatLength(tail.length()) + tail;
    }

public:

    // ---------------------------------------------------
    //                 INSERT DOCTOR
    // ---------------------------------------------------
    void insertDoctor(const string& name,
                      const string& address,
                      PrimaryIndex& doctorIndex)
    {
        if (doctorNameExists(name))
        {
            cout << "Error: Doctor name already exists.\n";
            return;
        }

        const string dataFile = "doctors.txt";
        const string availFile = "doctorsAvailList.txt";

        string dummyID = "00";
        string dummyTail = " |" + dummyID + "|" + name + "|" + address;
        int minLen = 3 + dummyTail.length();

        vector<Slot> avail = loadAvailList(availFile);
        int idx = -1;
        long off = -1;
        int slotLen = -1;

        bool foundSlot = findFirstFit(avail, minLen, idx, off, slotLen);

        string finalID;
        string record;
        long writeOffset;

        if (foundSlot)
        {
            finalID = readOldIDAtOffset(dataFile, off);

            record = buildDoctorRecord(finalID, name, address, slotLen);
            writeOffset = off;

            fstream file(dataFile, ios::in | ios::out);
            file.seekp(writeOffset);
            file.write(record.c_str(), record.length());
            file.close();

            avail.erase(avail.begin() + idx);
            saveAvailList(availFile, avail);

            long pos = doctorIndex.positionInVec(finalID);
            if (pos == -1)
                doctorIndex.addAndSort(finalID, writeOffset);
            else
                doctorIndex.indexList[pos].offset = writeOffset;

            doctorIndex.saveIndex();
        }
        else
        {
            string lastID = getLastIDFromFile(dataFile);
            finalID = formatID(idStringToInt(lastID) + 1);

            record = buildDoctorRecord(finalID, name, address);

            fstream file(dataFile, ios::in | ios::out | ios::ate);
            writeOffset = file.tellp();
            file << record << "\n";
            file.close();

            doctorIndex.addAndSort(finalID, writeOffset);
            doctorIndex.saveIndex();
        }

        cout << "Doctor inserted with ID: " << finalID << "\n";
    }

    // ---------------------------------------------------
    //              INSERT APPOINTMENT
    // ---------------------------------------------------
    void insertAppointment(const string& date,
                           const string& doctorID,
                           PrimaryIndex& appIndex)
    {
        if (!doctorIDValid(doctorID))
        {
            cout << "Error: Doctor ID does not exist or deleted.\n";
            return;
        }

        const string dataFile = "appointments.txt";
        const string availFile = "appointmentsAvailList.txt";

        string dummyID = "00";
        string dummyTail = " |" + dummyID + "|" + date + "|" + doctorID;
        int minLen = 3 + dummyTail.length();

        vector<Slot> avail = loadAvailList(availFile);
        int idx = -1;
        long off = -1;
        int slotLen = -1;

        bool foundSlot = findFirstFit(avail, minLen, idx, off, slotLen);

        string finalID;
        string record;
        long writeOffset;

        if (foundSlot)
        {
            finalID = readOldIDAtOffset(dataFile, off);

            record = buildAppointmentRecord(finalID, date, doctorID, slotLen);
            writeOffset = off;

            fstream file(dataFile, ios::in | ios::out);
            file.seekp(writeOffset);
            file.write(record.c_str(), record.length());
            file.close();

            avail.erase(avail.begin() + idx);
            saveAvailList(availFile, avail);

            long pos = appIndex.positionInVec(finalID);
            if (pos == -1)
                appIndex.addAndSort(finalID, writeOffset);
            else
                appIndex.indexList[pos].offset = writeOffset;

            appIndex.saveIndex();
        }
        else
        {
            string lastID = getLastIDFromFile(dataFile);
            finalID = formatID(idStringToInt(lastID) + 1);

            record = buildAppointmentRecord(finalID, date, doctorID);

            fstream file(dataFile, ios::in | ios::out | ios::ate);
            writeOffset = file.tellp();
            file << record << "\n";
            file.close();

            appIndex.addAndSort(finalID, writeOffset);
            appIndex.saveIndex();
        }

        cout << "Appointment inserted with ID: " << finalID << "\n";
    }
};




//
//
//
// Add Update Class here
//
//
//
//
//


// ====================== UPDATE MANAGER CLASS ======================


class UpdateManager {
private:
    // Format ID to 2 digits (01, 02, etc.)
    string formatID(const string& id) {
        if (id.length() == 1) return "0" + id;
        return id;
    }

    // Format length to 3 digits for record header
    string formatLength(int len) {
        string s = to_string(len);
        if (s.length() < 3)
            s = string(3 - s.length(), '0') + s;
        return s;
    }

    // Convert to lowercase for case-insensitive comparison
    string toLower(const string& str) {
        string result = str;
        transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    // Normalize name: lowercase and trim spaces
    string normalizeName(const string& name) {
        string result = toLower(name);
        size_t start = result.find_first_not_of(" ");
        size_t end = result.find_last_not_of(" ");
        if (start == string::npos || end == string::npos) return "";
        return result.substr(start, end - start + 1);
    }

    // Enforce field size limits [15], [30] as per assignment
    string enforceFieldSize(const string& field, int maxSize) {
        if (field.length() > maxSize) {
            return field.substr(0, maxSize);
        }
        return field;
    }

    // CORRECTED: Reliable duplicate checking that always works
    bool isDoctorNameExists(PrimaryIndex& doctorIndex, const string& newName, const string& excludeID = "") {
        string normalizedNewName = normalizeName(newName);
        if (normalizedNewName.empty()) return false;

        // CRITICAL: Reload index to ensure we have current data
        doctorIndex.loadIndex();

        for (const auto& entry : doctorIndex.indexList) {
            string record = doctorIndex.readRecordAtOffset(entry.offset);

            // Skip deleted records (marked with '*')
            if (record.empty() || record[3] == '*') continue;

            // Skip the doctor we're updating
            if (entry.id == excludeID) continue;

            // Parse record to extract name field
            size_t firstPipe = record.find('|');
            size_t secondPipe = record.find('|', firstPipe + 1);
            size_t thirdPipe = record.find('|', secondPipe + 1);

            if (firstPipe != string::npos && secondPipe != string::npos && thirdPipe != string::npos) {
                string existingName = record.substr(secondPipe + 1, thirdPipe - secondPipe - 1);
                string normalizedExistingName = normalizeName(existingName);

                // Case-insensitive comparison - EXACT MATCH REQUIRED
                if (normalizedExistingName == normalizedNewName) {
                    return true;
                }
            }
        }
        return false;
    }

    // Build doctor record with proper format and field sizes
    string buildDoctorRecord(const string& id, const string& name, const string& address) {
        // Enforce assignment field sizes
        string enforcedID = enforceFieldSize(id, 15);
        string enforcedName = enforceFieldSize(name, 30);
        string enforcedAddress = enforceFieldSize(address, 30);

        string tail = " |" + enforcedID + "|" + enforcedName + "|" + enforcedAddress;
        return formatLength(tail.length()) + tail;
    }

    // Build appointment record with proper format and field sizes
    string buildAppointmentRecord(const string& appID, const string& date, const string& doctorID) {
        // Enforce assignment field sizes
        string enforcedAppID = enforceFieldSize(appID, 15);
        string enforcedDate = enforceFieldSize(date, 30);
        string enforcedDoctorID = enforceFieldSize(doctorID, 15);

        string tail = " |" + enforcedAppID + "|" + enforcedDate + "|" + enforcedDoctorID;
        return formatLength(tail.length()) + tail;
    }

public:
    // CORRECTED: Update doctor name with guaranteed duplicate checking
    bool updateDoctorName(PrimaryIndex& doctorIndex, SecondaryIndexDoctorName& secName,
                          const string& doctorID, const string& newName) {

        // Input validation
        if (doctorID.empty() || newName.empty()) {
            cout << "Error: Doctor ID and name cannot be empty.\n";
            return false;
        }

        string formattedID = formatID(doctorID);

        // CRITICAL: Ensure index is current before any operations
        doctorIndex.loadIndex();

        // Check if doctor exists
        long offset = doctorIndex.indexByID(formattedID);
        if (offset == -1) {
            cout << "Error: Doctor ID " << formattedID << " not found.\n";
            return false;
        }

        // Read and validate current record
        string record = doctorIndex.readRecordAtOffset(offset);
        if (record.empty()) {
            cout << "Error: Cannot read doctor record.\n";
            return false;
        }

        // Prevent updates to deleted records
        if (record[3] == '*') {
            cout << "Error: Cannot update deleted doctor record.\n";
            return false;
        }

        // CORRECTED: Duplicate checking with current data
        if (isDoctorNameExists(doctorIndex, newName, formattedID)) {
            cout << "Error: Doctor name '" << newName << "' already exists in the system.\n";
            return false;
        }

        // Parse record to extract fields
        size_t firstPipe = record.find('|');
        size_t secondPipe = record.find('|', firstPipe + 1);
        size_t thirdPipe = record.find('|', secondPipe + 1);

        if (firstPipe == string::npos || secondPipe == string::npos || thirdPipe == string::npos) {
            cout << "Error: Invalid doctor record format.\n";
            return false;
        }

        // Extract current fields (update non-key fields only)
        string currentID = record.substr(firstPipe + 1, secondPipe - firstPipe - 1);
        string currentAddress = record.substr(thirdPipe + 1);

        // Build updated record with proper length indicator
        string updatedRecord = buildDoctorRecord(currentID, newName, currentAddress);

        // Write updated record to file
        fstream file("doctors.txt", ios::in | ios::out);
        if (!file) {
            cout << "Error: Cannot open doctors.txt\n";
            return false;
        }

        file.seekp(offset);
        file << updatedRecord;

        if (file.fail()) {
            cout << "Error: Failed to write updated record.\n";
            file.close();
            return false;
        }

        file.close();

        // Update secondary index as required by assignment
        secName.createIndex();

        cout << "Doctor " << formattedID << " name updated successfully!\n";
        return true;
    }

    // Update appointment date
    bool updateAppointmentDate(PrimaryIndex& appIndex, SecondaryIndexDoctorID& secID,
                               const string& appointmentID, const string& newDate) {

        // Input validation
        if (appointmentID.empty() || newDate.empty()) {
            cout << "Error: Appointment ID and date cannot be empty.\n";
            return false;
        }

        string formattedID = formatID(appointmentID);

        // Check if appointment exists
        long offset = appIndex.indexByID(formattedID);
        if (offset == -1) {
            cout << "Error: Appointment ID " << formattedID << " not found.\n";
            return false;
        }

        // Read and validate current record
        string record = appIndex.readRecordAtOffset(offset);
        if (record.empty()) {
            cout << "Error: Cannot read appointment record.\n";
            return false;
        }

        // Prevent updates to deleted records
        if (record[3] == '*') {
            cout << "Error: Cannot update deleted appointment record.\n";
            return false;
        }

        // Parse record to extract fields
        size_t firstPipe = record.find('|');
        size_t secondPipe = record.find('|', firstPipe + 1);
        size_t thirdPipe = record.find('|', secondPipe + 1);

        if (firstPipe == string::npos || secondPipe == string::npos || thirdPipe == string::npos) {
            cout << "Error: Invalid appointment record format.\n";
            return false;
        }

        // Extract current fields (update non-key fields only)
        string currentAppID = record.substr(firstPipe + 1, secondPipe - firstPipe - 1);
        string currentDoctorID = record.substr(thirdPipe + 1);

        // Build updated record with proper length indicator
        string updatedRecord = buildAppointmentRecord(currentAppID, newDate, currentDoctorID);

        // Write updated record to file
        fstream file("appointments.txt", ios::in | ios::out);
        if (!file) {
            cout << "Error: Cannot open appointments.txt\n";
            return false;
        }

        file.seekp(offset);
        file << updatedRecord;

        if (file.fail()) {
            cout << "Error: Failed to write updated record.\n";
            file.close();
            return false;
        }

        file.close();

        // Update secondary index as required by assignment
        secID.createIndex();

        cout << "Appointment " << formattedID << " date updated successfully!\n";
        return true;
    }
};
struct FreeSlot
{
    long offset;
    int length;
};

class DeleteManager
{
private:
    vector<FreeSlot> appointmentsAvailList;
    vector<FreeSlot> doctorsAvailList;

public:

    DeleteManager()
    {
        loadAvailList("appointmentsAvailList.txt", appointmentsAvailList);
        loadAvailList("doctorsAvailList.txt", doctorsAvailList);
    }
    int getRecordLength(long offset, const string& filename)
    {
        ifstream file(filename);
        if (!file)
            return -1;

        file.seekg(offset);

        string record;
        getline(file, record);

        return record.length();
    }
    // load data from file to vector
    void loadAvailList(const string& filename, vector<FreeSlot>& list)
    {
        ifstream file(filename);
        if (!file) return;

        long offset;
        int length;

        while (file >> offset >> length)
        {
            list.push_back({ offset, length });
        }

        file.close();
    }

    // save deleted record data to avail file
    void appendToFile(const string& filename, const FreeSlot& slot)
    {
        ofstream file(filename, ios::app);
        if (!file) return;

        file << slot.offset << " " << slot.length << "\n";
        file.close();
    }


    bool deleteAppointment(PrimaryIndex& appIndex, const string& appID)
    {
        long offset = appIndex.indexByID(appID);
        if (offset == -1)
        {
            cout << "Warning: Appointment ID not found.\n";
            return false;
        }

        fstream file("appointments.txt", ios::in | ios::out);
        if (!file)
        {
            cout << "Error opening appointments.txt\n";
            return false;
        }

        file.seekg(offset);
        char header[4];
        file.read(header, 4);
        if (header[3] == '*')
        {
            cout << "Warning: Appointment already deleted.\n";
            file.close();
            return false;
        }

        file.seekp(offset + 3);
        file.put('*');

        int recSize = getRecordLength(offset, "appointments.txt");

        if (recSize > 0)
        {
            FreeSlot slot = { offset, recSize };
            appointmentsAvailList.push_back(slot);
            appendToFile("appointmentsAvailList.txt", slot);
        }

        cout << "Appointment " << appID << " deleted.\n";
        file.close();
        return true;
    }


    bool deleteDoctor(PrimaryIndex& doctorIndex, const string& docID)
    {
        long offset = doctorIndex.indexByID(docID);
        if (offset == -1)
        {
            cout << "Warning: Doctor ID not found.\n";
            return false;
        }

        fstream file("doctors.txt", ios::in | ios::out);
        if (!file)
        {
            cout << "Error opening doctors.txt\n";
            return false;
        }

        file.seekg(offset);
        char header[4];
        file.read(header, 4);
        if (header[3] == '*')
        {
            cout << "Warning: Appointment already deleted.\n";
            file.close();
            return false;
        }

        file.seekp(offset + 3);
        file.put('*');

        int recSize = getRecordLength(offset, "doctors.txt");

        if (recSize > 0)
        {
            FreeSlot slot = { offset, recSize };
            doctorsAvailList.push_back(slot);
            appendToFile("doctorsAvailList.txt", slot);
        }

        cout << "Doctor " << docID << " deleted.\n";
        file.close();
        return true;
    }

    void printAvailLists()
    {
        cout << "\n--- Appointments Avail List (Variable-Length) ---\n";
        if (appointmentsAvailList.empty())
        {
            cout << "No deleted appointment records.\n";
        }
        else
        {
            for (auto slot : appointmentsAvailList)
                cout << "Offset: " << slot.offset << " | Size: " << slot.length << endl;
        }

        cout << "\n--- Doctors Avail List (Variable-Length) ---\n";
        if (doctorsAvailList.empty())
        {
            cout << "No deleted doctor records.\n";
        }
        else
        {
            for (auto slot : doctorsAvailList)
                cout << "Offset: " << slot.offset << " | Size: " << slot.length << endl;
        }
    }
};


class InfoManager
{
public:

    void printDoctorInfo(PrimaryIndex& docIndex, const string& doctorID)
    {
        long offset = docIndex.indexByID(doctorID);

        if (offset == -1)
        {
            cout << "Doctor ID not found.\n";
            return;
        }

        ifstream file("doctors.txt");
        if (!file)
        {
            cout << "Error opening doctors.txt\n";
            return;
        }

        file.seekg(offset);

        string record;
        getline(file, record);

        file.close();
        if (record[3] == '*')
        {
            cout << "This record is deleted.\n";
            return;
        }

        cout << "\n=== Doctor Info ===\n";
        cout << record << endl;
    }


    void printAppointmentInfo(PrimaryIndex& appIndex, const string& appID)
    {
        long offset = appIndex.indexByID(appID);

        if (offset == -1)
        {
            cout << "Appointment ID not found.\n";
            return;
        }

        ifstream file("appointments.txt");
        if (!file)
        {
            cout << "Error opening appointments.txt\n";
            return;
        }

        file.seekg(offset);

        string record;
        getline(file, record);

        file.close();
        if (record[3] == '*')
        {
            cout << "This record is deleted.\n";
            return;
        }

        cout << "\n=== Appointment Info ===\n";
        cout << record << endl;
    }
};


class QueryManager
{
public:

    void executeQuery(string query, PrimaryIndex& doctorPrimary, PrimaryIndex& appPrimary,
                      SecondaryIndexDoctorID& secDocID, SecondaryIndexDoctorName& secDocName
    )
    {
        string q = toLower(query);

        // Query 1: Select all from Doctors where Doctor ID='xxx';
        if (q.find("select all from doctors") != string::npos &&
            q.find("doctor id") != string::npos)
        {
            string id = getValueBetweenQuotes(query);
            printDoctorByID(doctorPrimary, id);
            return;
        }

        // Query 2: Select all from Appointments where Doctor ID='xxx';
        if (q.find("select all from appointments") != string::npos &&
            q.find("doctor id") != string::npos)
        {
            string id = getValueBetweenQuotes(query);
            printAppointmentByDoctorID(secDocID, id);
            return;
        }

        // Query 3: Select Doctor Name from Doctors where Doctor Name='xxx';
        if (q.find("select doctor name from doctors") != string::npos &&
            q.find("doctor name") != string::npos)
        {
            string name = getValueBetweenQuotes(query);
            printDoctorByName(secDocName, name);
            return;
        }

        cout << "Invalid query format.\n";
    }

private:

    string toLower(string s)
    {
        for (char& c : s)
            c = tolower(c);
        return s;
    }

    string getValueBetweenQuotes(const string& q)
    {
        int f = q.find("'");
        int l = q.find_last_of("'");

        if (f == -1 || l == -1 || l <= f)
            return "";

        return q.substr(f + 1, l - f - 1);
    }

    // Query 1: Using Primary Index (Doctors)
    void printDoctorByID(PrimaryIndex& idx, const string& id)
    {
        long offset = idx.indexByID(id);

        if (offset == -1)
        {
            cout << "Doctor not found.\n";
            return;
        }

        ifstream file("doctors.txt");
        if (!file)
        {
            cout << "Error opening doctors.txt\n";
            return;
        }

        file.seekg(offset);

        string record;
        getline(file, record);

        file.close();

        if (record.size() > 3 && record[3] == '*')
        {
            cout << "This record is deleted.\n";
            return;
        }

        cout << "\n=== Result ===\n" << record << "\n";
    }


    // Query 2: Using Secondary Index (Appointments)
    void printAppointmentByDoctorID(SecondaryIndexDoctorID& sec, const string& id)
    {
        vector<pair<string, long>> results = sec.searchByID(id);

        if (results.empty())
        {
            cout << "No appointments found for this doctor.\n";
            return;
        }

        cout << "\n=== Appointments for Doctor " << id << " ===\n";


        for (auto& p : results)
        {
            long offset = p.second;
            string record = sec.getRecordAtOffset(offset);

            if (record.empty())
                continue;
            if (record.size() > 3 && record[3] == '*')
            {
                cout << "[DELETED RECORD]\n";
                continue;
            }
            cout << record << endl;
        }
    }


    // Query 3: Using Secondary Index (Doctors by Name)
    void printDoctorByName(SecondaryIndexDoctorName& sec, const string& name)
    {

        auto result = sec.searchByName(name);

        if (result.first == -1)
        {
            cout << "Doctor name not found.\n";
            return;
        }

        string record = result.second;
        if (record.size() > 3 && record[3] == '*')
        {
            cout << "[DELETED RECORD]\n";
            return;
        }
        cout << "\n=== Result ===\n" << record << "\n";
    }
};


int main()
{
    PrimaryIndex appIndex("AppointmentsIndexfile.txt", "appointments.txt");
    appIndex.loadIndex();

    PrimaryIndex doctorIndex("DocIndexFile.txt", "doctors.txt");
    doctorIndex.loadIndex();


    SecondaryIndexDoctorID   secID("SecondryIndex_DoctorId_App.txt", "appointments.txt");
    SecondaryIndexDoctorName secName("SecondryIndex_DoctorName.txt", "doctors.txt");


    QueryManager qm;
    DeleteManager dm;
    InfoManager info;
    Insert ins;

    UpdateManager um;

    int choice;

    string id, newName, newDate;

    do
    {
        secID.createIndex();
        secName.createIndex();

        secID.loadIndex();
        secName.loadIndex();

        cout << "1. Add New Doctor\n";
        cout << "2. Add New Appointment\n";
        cout << "3. Update Doctor Name (Doctor ID)\n";
        cout << "4. Update Appointment Date (Appointment ID)\n";
        cout << "5. Delete Appointment (Appointment ID)\n";
        cout << "6. Delete Doctor (Doctor ID)\n";
        cout << "7. Print Doctor Info (Doctor ID)\n";
        cout << "8. Print Appointment Info (Appointment ID)\n";
        cout << "9. Write Query\n";
        cout << "10. Exit\n";

        cout << "\nEnter choice: ";
        cin >> choice;

        switch (choice)
        {
            case 1:
            {
                string name, address;
                cout << "Enter Doctor Name: ";
                cin >> ws;              // ???? ????????/??? \n
                getline(cin, name);

                cout << "Enter Doctor Address: ";
                getline(cin, address);

                ins.insertDoctor(name, address, doctorIndex);
            }
                break;

            case 2:
            {
                string date, docID;
                cout << "Enter Appointment Date: ";
                cin >> ws;
                getline(cin, date);

                cout << "Enter Doctor ID: ";
                cin >> docID;

                ins.insertAppointment(date, docID, appIndex);

            }
                break;

            case 3:


                cout << "Enter Doctor ID to update: ";
                cin >> id;
                cin.ignore();
                cout << "Enter New Name: ";
                getline(cin, newName);
                um.updateDoctorName(doctorIndex, secName, id, newName);
                break;

            case 4:


                cout << "Enter Appointment ID to update: ";
                cin >> id;
                cin.ignore();
                cout << "Enter New Date: ";
                getline(cin, newDate);
                um.updateAppointmentDate(appIndex, secID, id, newDate);
                break;

            case 5:
                cout << "Enter Appointment ID to delete: ";
                cin >> id;
                dm.deleteAppointment(appIndex, id);
                break;

            case 6:
                cout << "Enter Doctor ID to delete: ";
                cin >> id;
                dm.deleteDoctor(doctorIndex, id);
                break;

            case 7:
                cout << "Enter Doctor ID: ";
                cin >> id;
                info.printDoctorInfo(doctorIndex, id);
                break;

            case 8:
                cout << "Enter Appointment ID: ";
                cin >> id;
                info.printAppointmentInfo(appIndex, id);
                break;

            case 9:
            {
                string query;
                cin.ignore();
                cout << "Enter Query: ";
                getline(cin, query);
                qm.executeQuery(query, doctorIndex, appIndex, secID, secName);
            }
                break;


            case 10:
                cout << "Exiting...\n";
                break;

            default:
                cout << "Invalid choice.\n";
        }

    } while (choice != 10);

    dm.printAvailLists();

    return 0;
}
