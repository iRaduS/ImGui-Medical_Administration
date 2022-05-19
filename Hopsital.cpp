// Check for plots: https://news.t0.vc/YVPE
#define _CRT_SECURE_NO_WARNINGS
#define DEBUG_ID 1

#include <algorithm>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_opengl3_loader.h>
#include <GLFW/glfw3.h>
#include <implot.h>
#include <implot_internal.h>
#include <mysql/mysql.h>
#include <exception>
#include <string>
#include <iostream>
#include <list>
#include <vector>
#include <unordered_map>
#include <regex>
using namespace std;
class Pacient;
class CadruMedical;
class Medic;
class Asistent;
class AdministrareMedicamente;
class Consultari;
class HeartbeatRate;
list <Pacient*> gPacient;
list <CadruMedical*> gCadreMedicale;
list <AdministrareMedicamente*> gAdministrareMedicamente;
list <Consultari*> gConsultari;
MYSQL* handler;

// define a time data structure and time interval structure
struct TimeData {
    int hour, minute, second;
};
typedef pair<TimeData, TimeData> TimeInterval;
// interface global
bool showWindow = true, showAssistents = false, showPacients = true, showSpecialityMedics = false, showPrimaryMedics = false;
// default pacients menu;
int weight = 80, age = 18, height = 160, pacientSelectedItem = 0;
string pacientName, reason, saveError = "";
// local pacient menu;
bool localPacientSetInfo = false;
int localPacientWeight, localPacientAge, localPacientHeight, localPacientSistole, localPacientDiastol, localPacientPulse;
string localPacientName, localPacientReason, localPacientError = "";
// local cadru medical menu;
bool CMRezident;
string CMName, CMDepartament, CMError = "", CMSpecializare, CMWorkingInterval;
int CMWorkingYears = 2, localCMSelected;
list<Pacient*> CMPacients;
// local cadru medical
bool localCMSetInfo = false;
bool localCMRezident;
CadruMedical *localEdit;
string localCMName, localCMDepartament, localCMError = "", localCMSpecializare, localCMWorkingInterval;
int localCMWorkingYears = 2;
list<Pacient*> localCMPacients;
// administrare Medicament
string medicamentName, medicationError;
TimeData administrationTime;
Pacient* administree;
AdministrareMedicamente* currentSelection;
Asistent* administrator;
int typeOfAdministration;
string cError;
TimeData consultation;
Pacient *cPacient;
Medic *cMedic;
Consultari *cSelection;

template<class structure, class T> 
inline bool contains(structure C, const T& Value)
{
    return std::find(C.begin(), C.end(), Value) != C.end();
}

string GetInstanceType(int type) {
    switch (type) {
    case 0:
        return "asistent";
    case 1:
        return "medic";
    case 2:
        return "medic primar";
    default:
        throw runtime_error("Tipul oferit nu este bun incearca intre [0 - 2].");
    }
}

// database related exception class
class DatabaseException : public exception {
protected:
    string msg;
public:
    explicit DatabaseException(const char* message) : msg(message) {}
    explicit DatabaseException(const string& message) : msg(message) {}
    virtual ~DatabaseException() noexcept {}
    virtual const char* what() const noexcept {
        return msg.c_str();
    }
};

// gui related exception class
class GUIException : public exception {
protected:
    int* addressLocation;
    string msg;
public:
    explicit GUIException(const char* message, int* const addressLocation = NULL) : msg(message), addressLocation(addressLocation) {}
    explicit GUIException(const string& message, int* const addressLocation = NULL) : msg(message), addressLocation(addressLocation) {}
    virtual ~GUIException() noexcept {}
    virtual const char* what() const noexcept {
        if (addressLocation != NULL) {
            const_cast<string&>(msg) = const_cast<string&>(msg) + " at address: " + to_string(*addressLocation);
            return msg.c_str();
        }

        return msg.c_str();
    }
};

class DB {
private:
    DB(MYSQL* handler) : handler(handler) {}
    static DB* instance;
    MYSQL* handler;
public:
    DB(DB& db) = delete;
    void operator=(const DB&) = delete;

    static DB* GetInstance(MYSQL* handler) {
        if (!instance) {
            instance = new DB(handler);
        }
        return instance;
    }

    vector<MYSQL_ROW> fetchAll(const char* table) {
        vector<MYSQL_ROW> persistanceResults;
        string query = "SELECT * FROM " + string(table);
        int status = mysql_query(handler, query.c_str());

        if (!status) {
            MYSQL_RES* res = mysql_store_result(handler);

            MYSQL_ROW row;
            while (row = mysql_fetch_row(res)) {
                persistanceResults.push_back(row);
            }
        }

        return persistanceResults;
    }

    bool deleteById(const char* table, int id) {
        string query = "DELETE FROM " + string(table) + " WHERE id = " + to_string(id);

        int status = mysql_query(handler, query.c_str());

        if (status == 0) {
            return true;
        }
        else {
            return false;
        }
    }

    bool insertable(const char* table, vector<pair<string, string>> queryBuilder) {
        string query = "INSERT INTO " + string(table) + " (";
        
        int k = 0;
        for (auto it = queryBuilder.begin(); it != queryBuilder.end(); it++) {
            if (k == queryBuilder.size() - 1) {
                query += (it->first);
            }
            else {
                query += (it->first + ", ");
            }

            k++;
        }

        query += ") VALUES (";
        k = 0;
        for (auto it = queryBuilder.begin(); it != queryBuilder.end(); it++) {
            if (k == queryBuilder.size() - 1) {
                query += ("'" + (it->second) + "'");
            }
            else {
                query += ("'" + (it->second) + "'" + ", ");
            }

            k++;
        }
        query += ")";

        cout << query << endl;

        int status = mysql_query(handler, query.c_str());
        if (status == 0) {
            return true;
        }
        else {
            return false;
        }
    }

    int getLatestId(const char* table) {
        string query = "SELECT IFNULL(id, 1) FROM " + string(table) + " ORDER BY id DESC LIMIT 1";
        int status = mysql_query(handler, query.c_str());

        if (!status) {
            MYSQL_RES* res = mysql_store_result(handler);

            MYSQL_ROW row;
            while (row = mysql_fetch_row(res)) {
                return atoi(row[0]);
            }
        }
    }
};
DB* DB::instance = NULL;

// template class used to validate
template <class T>
class Validator {
public:
    Validator() = default;
    virtual ~Validator() = default;

    static void throwIfNotPositive(T);
    static void throwIfNotStrictPositive(T);
    static void throwIfNotInRange(T, T, T, string = "Number");
    static void throwIfGreaterThan(T, T, string = "Number");
    static void throwIfEmptyOrNull(T, string = "Number");
    static void throwIfTimeIntervalIncorrect(T, string = "Interval");
    static void throwIfNull(T, string = "Object");
};

template <class T>
void Validator<T>::throwIfNotPositive(T value) {
    if (value < 0) {
        throw runtime_error("Number must be positive!");
    }
}

template <class T>
void Validator<T>::throwIfEmptyOrNull(T value, string ob_name) {
    if (&value == NULL || !value.size()) {
        throw runtime_error(ob_name + " is empty.");
    }
}

template <class T>
void Validator<T>::throwIfNull(T value, string ob_name) {
    if (value == NULL) {
        throw runtime_error(ob_name + " is null.");
    }
}

template <class T>
void Validator<T>::throwIfTimeIntervalIncorrect(T value, string ob_name) {
    regex reQuery("[0-9][0-9]:[0-9][0-9]:[0-9][0-9]-[0-9][0-9]:[0-9][0-9]:[0-9][0-9]");
    if (!regex_match(value, reQuery)) {
        throw runtime_error(ob_name + " incorrect formated.");
    }

    TimeInterval timeInterval;
    (void)sscanf(value.c_str(), "%02d:%02d:%02d - %02d:%02d:%02d", &timeInterval.first.hour,
        &timeInterval.first.minute, &timeInterval.first.second, &timeInterval.second.hour, &timeInterval.second.minute, &timeInterval.second.second);

    if (timeInterval.first.hour > 24 || timeInterval.first.hour < 0 || timeInterval.second.hour > 24 || timeInterval.second.hour < 0) {
        throw runtime_error("Hour is incorrect.");
    }
    if (timeInterval.first.minute > 59 || timeInterval.first.minute < 0 || timeInterval.second.minute > 59 || timeInterval.second.minute < 0) {
        throw runtime_error("Minute is incorrect.");
    }
    if (timeInterval.first.second > 59 || timeInterval.first.second < 0 || timeInterval.second.second > 59 || timeInterval.second.second < 0) {
        throw runtime_error("Second is incorrect.");
    }
}

template <class T>
void Validator<T>::throwIfNotStrictPositive(T value) {
    if (value <= 0) {
        throw runtime_error("Number must be strictly positive!");
    }
}

template <class T>
void Validator<T>::throwIfNotInRange(T value, T a, T b, string ob_name) {
    if (value < a || value > b) {
        throw runtime_error(ob_name + " must be inside the interval [" + to_string(a) + ", " + to_string(b) + "]");
    }
}

template <class T>
void Validator<T>::throwIfGreaterThan(T value, T a, string ob_name) {
    if (value > a) {
        throw runtime_error(ob_name + " must be smaller than " + to_string(a));
    }
}

// Interface from lab
class StreamInterface {
public:
    StreamInterface() = default;
    virtual ~StreamInterface() = default;

    friend ostream& operator<<(std::ostream& out, const StreamInterface& ob) {
        ob.write(out);
        return out;
    }

    virtual void write(ostream& out) const = 0;
};

class Pacient : public StreamInterface {
private:
    const int ID;

    string fullName, reason;
    int age, height, weight;

    vector<HeartbeatRate*> heartbeatStats;
public:
    Pacient() = default;
    Pacient(int, int = ::age, int = ::height, int = ::weight, const string = ::pacientName, const string = ::reason, const vector<HeartbeatRate*>& = vector<HeartbeatRate*>());
    Pacient(const Pacient&);
    ~Pacient();

    string getFullName() const;
    int getId() const;
    string getReason() const;
    int getAge() const;
    int getHeight() const;
    int getWeight() const;
    vector<HeartbeatRate*> getHeartbeatStats() const;
    void setHeartbeatStats(vector<HeartbeatRate*>);
    void setName(string name) {
        fullName = name;
    }
    void setReason(string res) {
        reason = res;
    }
    void setAge(int a) {
        age = a;
    }
    void setWeight(int w) {
        weight = w;
    }
    void setHeight(int h) {
        height = h;
    }

    Pacient& operator=(const Pacient&);
    explicit operator string() const;
    
    void write(ostream&) const override;
    pair<ImVec4, string> computeIMCReport() const;
};

Pacient::Pacient(int id, int age, int height, int weight, const string pacientName, const string reason, const vector<HeartbeatRate*>& heartbeatStats) :
    ID(id), age(age), height(height), weight(weight), fullName(pacientName), reason(reason), heartbeatStats(heartbeatStats) {
    Validator<string>::throwIfEmptyOrNull(fullName, "Pacient's name");
    Validator<string>::throwIfEmptyOrNull(reason, "Incoming reason");
}

Pacient::Pacient(const Pacient& pacient) : ID(pacient.ID), age(pacient.age), height(pacient.height), weight(pacient.weight), fullName(pacient.fullName), reason(pacient.reason), heartbeatStats(pacient.heartbeatStats) {
    Validator<string>::throwIfEmptyOrNull(fullName, "Pacient's name");
    Validator<string>::throwIfEmptyOrNull(reason, "Incoming reason");
}

Pacient::operator string() const {
    return fullName + " - motiv: " + reason;
}

pair<ImVec4, string> Pacient::computeIMCReport() const {
    if (age < 18 || age > 65) {
        return { ImVec4(255, 0, 0, 255), "Nu se aplica (pacientul nu are varsta potrivita)!" };
    }
    const double imcIndex = weight / pow((height / 100.0), 2);

    if (imcIndex < 18.5) {
        return { ImVec4(255, 255, 255, 255), "Subponderal (indice: " + to_string(imcIndex) + ")" };
    }
    else if (imcIndex >= 18.5 && imcIndex < 25.0) {
        return { ImVec4(0, 255, 0, 255), "Normal (indice: " + to_string(imcIndex) + ")" };
    }
    else if (imcIndex >= 25.0 && imcIndex < 30.0) {
        return { ImVec4(1.0f, 0.8f, 0.6f, 1.00f), "Supraponderal (indice: " + to_string(imcIndex) + ")" };
    }
    else if (imcIndex >= 30.0 && imcIndex < 35.0) {
        return { ImVec4(1.0f, 0.6f, 0.4f, 1.00f), "Obezitate (gradul I) (indice: " + to_string(imcIndex) + ")" };
    }
    else if (imcIndex >= 35.0 && imcIndex < 40.0) {
        return { ImVec4(1.0f, 0.4f, 0.2f, 1.00f), "Obezitate (gradul II) (indice: " + to_string(imcIndex) + ")" };
    }
    else {
        return { ImVec4(255, 0, 0, 255), "Obezitate morbida (indice: " + to_string(imcIndex) + ")" };
    }
}

Pacient& Pacient::operator=(const Pacient& pacient) {
    if (this != &pacient) {
        age = pacient.age, height = pacient.height, weight = pacient.weight,
        fullName = pacient.fullName, reason = pacient.reason, heartbeatStats = pacient.heartbeatStats;
    }
    return *this;
}

string Pacient::getFullName() const {
    return fullName;
}

string Pacient::getReason() const {
    return reason;
}

int Pacient::getId() const {
    return ID;
}

int Pacient::getAge() const {
    return age;
}

int Pacient::getWeight() const {
    return weight;
}

int Pacient::getHeight() const {
    return height;
}

vector<HeartbeatRate*> Pacient::getHeartbeatStats() const {
    return heartbeatStats;
}

void Pacient::setHeartbeatStats(vector<HeartbeatRate*> heartbeatRate) {
    heartbeatStats = heartbeatRate;
}

void Pacient::write(ostream& out) const {
    out << "[DEBUG]: Un pacient nou a fost creat: " << fullName
        << " (v: " << age << ", h: " << height << ", w: " << weight << ") "
        << "motiv: " << reason << '\n';
}

class CadruMedical : public StreamInterface {
protected:
    const int ID;
    string fullName, departament;

    list<Pacient*> pacients;
    TimeInterval workingInterval;

    int workingYears;
    bool isResident;
public:
    CadruMedical() : ID(0) {};
    CadruMedical(int, string = "Nedeterminat", string = "General", const list<Pacient*> & = list<Pacient*>(), TimeInterval = { {0, 0, 0}, {0, 0, 0} }, int = 0, bool = false);
    virtual ~CadruMedical() = default;

    bool getRezident() const {
        return isResident;
    }
    string getDepartament() const {
        return departament;
    }
    TimeInterval getTimeInterval() const {
        return workingInterval;
    }
    int getWorkingYears() const {
        return workingYears;
    }
    list<Pacient*> getPacients() const {
        return pacients;
    }
    string getFullName() const {
        return fullName;
    }
    int getId() const {
        return ID;
    }

    void setRezident(bool resident) {
        isResident = resident;
    }
    void setDepartament(string departament) {
        this->departament = departament;
    }
    void setTimeInterval(TimeInterval timeInterval) {
        workingInterval = timeInterval;
    }
    void setWorkingYears(int workingYears) {
        this->workingYears = workingYears;
    }
    void setPacients(const list<Pacient*>& pacients) {
        this->pacients = pacients;
    }
    void setFullName(string fullName) {
        this->fullName = fullName;
    }
};
CadruMedical::CadruMedical(int ID, string fullName, string departament, const list<Pacient*>& pacients, TimeInterval workingInterval, int workingYears, bool isResident) :
    ID(ID), fullName(fullName), departament(departament), pacients(pacients), workingInterval(workingInterval), workingYears(workingYears), isResident(isResident) {}

class Asistent : virtual public CadruMedical {
public:
    ~Asistent();
    Asistent() = default;
    Asistent(int, string = "Nedeterminat", string = "General", const list<Pacient*> & = list<Pacient*>(), TimeInterval = { {0, 0, 0}, {0, 0, 0} }, int = 0, bool = false);

    void write(ostream& out) const {
        out << "[DEBUG]: Un asistent nou a fost creat: " << fullName
            << " (d: " << departament << ", w_y: " << workingYears << ", resident: " << isResident << ").\n";
    }
};
Asistent::Asistent(int ID, string fullName, string departament, const list<Pacient*>& pacients, TimeInterval workingInterval, int workingYears, bool isResident)
    : CadruMedical(ID, fullName, departament, pacients, workingInterval, workingYears, isResident) {}


class Medic : virtual public CadruMedical {
protected:
    string specializare;
public:
    Medic() = default;
    ~Medic();
    Medic(int, string = "Nedeterminat", string = "General", const list<Pacient*> & = list<Pacient*>(), TimeInterval = { {0, 0, 0}, {0, 0, 0} }, int = 0, bool = false, string = "Nedeterminata");

    string getSpecializare() const {
        return specializare;
    }

    virtual void setSpecializare(string specializare) {
        this->specializare = specializare;
    }

    void write(ostream& out) const {
        out << "[DEBUG]: Un medic nou a fost creat: " << fullName
            << " (d: " << departament << ", w_y: " << workingYears << ", resident: " 
            << isResident << ", specializare: "<< specializare << ").\n";
    }
};
Medic::Medic(int ID, string fullName, string departament, const list<Pacient*>& pacients, TimeInterval workingInterval, int workingYears, bool isResident, string specializare)
    : CadruMedical(ID, fullName, departament, pacients, workingInterval, workingYears, isResident), specializare(specializare) {}

class MedicPrimar : public Asistent, public Medic {
private:
    list<CadruMedical*> residentTeam;
public:
    MedicPrimar() = default;
    ~MedicPrimar() = default;
    MedicPrimar(int, string = "Nedeterminat", string = "General", const list<Pacient*> & = list<Pacient*>(), TimeInterval = { {0, 0, 0}, {0, 0, 0} }, int = 0, bool = false, string = "Nedeterminata", const list<CadruMedical*>& = list<CadruMedical*>());
    
    list<CadruMedical*> getResidentTeam() const {
        return residentTeam;
    }

    void setResidentTeam(const list<CadruMedical*>& reziTeam) {
        residentTeam = reziTeam;
    }

    void write(ostream& out) const {
        out << "[DEBUG]: Un medic primar nou a fost creat: " << fullName
            << " (d: " << departament << ", w_y: " << workingYears << ", specializare: " 
            << specializare << ").\n";
    }
};
MedicPrimar::MedicPrimar(int ID, string fullName, string departament, const list<Pacient*>& pacients, TimeInterval workingInterval, int workingYears, bool isResident, string specializare, const list<CadruMedical*>& residentTeam)
    : Asistent(ID, fullName, departament, pacients, workingInterval, workingYears, isResident), Medic(ID, fullName, departament, pacients, workingInterval, workingYears, isResident, specializare), CadruMedical(ID, fullName, departament, pacients, workingInterval, workingYears, isResident), residentTeam(residentTeam) {}

class AdministrareMedicamente {
private:
    const int ID;

    Pacient* pacient;
    Asistent* asistent;

    string medicineName;
    TimeData administrationHour;

    unsigned int typeOfAdministration;
public:
    AdministrareMedicamente() : ID(0) {};
    AdministrareMedicamente(int, string = ::medicamentName, TimeData = ::administrationTime, unsigned int = ::typeOfAdministration, Pacient* = ::administree, Asistent* = ::administrator);
    explicit operator string() const {
        if (asistent && pacient) {
            return "(CM): " + asistent->getFullName() + " -> (P): " + pacient->getFullName();
        }
        return "";
    }
    string getAllInformation() const {
        string administration = typeOfAdministration == 0 ? "orala" : typeOfAdministration == 1 ? "intramusculara" : "intravenoasa";

        if (asistent && pacient) {
            return "CM " + asistent->getFullName() + " a administrat pe cale " + administration + " P " + pacient->getFullName() + " " + medicineName;
        }
        return "";
    }
    int getId() const {
        return ID;
    }
    Asistent* getAsistent() const {
        return asistent;
    }
    Pacient* getPacient() const {
        return pacient;
    }
};
AdministrareMedicamente::AdministrareMedicamente(int ID, string medicineName, TimeData administrationHour, unsigned int typeOfAdministration, Pacient* pacient, Asistent* asistent)
    : ID(ID), medicineName(medicineName), administrationHour(administrationHour), typeOfAdministration(typeOfAdministration), pacient(pacient), asistent(asistent) {}


class Consultari {
private:
    const int ID;

    Pacient* pacient;
    Medic* medic;

    TimeData consultationHour;
public:
    Consultari() : ID(0) {}
    ~Consultari() = default;
    Consultari(int, TimeData = ::consultation, Pacient* = ::cPacient, Medic* = ::cMedic);
    explicit operator string() const {
        if (medic && pacient) {
            return "(CM): " + medic->getFullName() + " -> (P): " + pacient->getFullName();
        }
        return "";
    }
    string getAllInformation() const {
        string data;
        sprintf(const_cast<char*>(data.c_str()), "%02d:%02d:%02d", consultationHour.hour, consultationHour.minute, consultationHour.second);

        if (medic && pacient) {
            return "CM " + medic->getFullName() + " (specializarea: " + medic->getSpecializare() + ") are o consultatie cu " + pacient->getFullName() + " la ora: " + data.c_str();
        }
        return "";
    }
    Medic* getMedic() const {
        return medic;
    }
    Pacient* getPacient() const {
        return pacient;
    }
    int getId() const {
        return ID;
    }
};
Consultari::Consultari(int ID, TimeData consultationHour, Pacient* pacient, Medic* medic) : ID(ID), consultationHour(consultationHour), pacient(pacient), medic(medic) {}


class HeartbeatRate {
private:
    int sistole, diastole, pulse;
public:
    ~HeartbeatRate() = default;
    HeartbeatRate(int = 120, int = 60, int = 90);

    int getSistole() const;
    int getDiastole() const;
    int getPulse() const;
};
HeartbeatRate::HeartbeatRate(int sistole, int diastole, int pulse) : sistole(sistole), diastole(diastole), pulse(pulse) {}
int HeartbeatRate::getSistole() const {
    return sistole;
}
int HeartbeatRate::getDiastole() const {
    return diastole;
}
int HeartbeatRate::getPulse() const {
    return pulse;
}

Medic::~Medic() {
    auto iterator = find_if(gConsultari.begin(), gConsultari.end(), [this](auto consult) -> bool {
        return consult->getMedic()->getId() == this->getId();
    });

    for (auto consultatie : gConsultari) {
        if (consultatie == *iterator) {
            gConsultari.remove(consultatie);

            if (consultatie != NULL) {
                delete consultatie;
            }

            break;
        }
    }
}

Asistent::~Asistent() {
    auto iterator = find_if(gAdministrareMedicamente.begin(), gAdministrareMedicamente.end(), [this](auto administrare) -> bool {
        return administrare->getAsistent()->getId() == this->getId();
    });

    for (auto administrare : gAdministrareMedicamente) {
        if (administrare == *iterator) {
            gAdministrareMedicamente.remove(administrare);

            if (administrare != NULL) {
                delete administrare;
            }

            break;
        }
    }
}

Pacient::~Pacient() {
    for (HeartbeatRate* heartbeatRate : heartbeatStats) {
        if (heartbeatRate != NULL) {
            delete heartbeatRate;
        }
    }

    auto iterator = find_if(gAdministrareMedicamente.begin(), gAdministrareMedicamente.end(), [this](auto administrare) -> bool {
        return administrare->getAsistent()->getId() == this->getId();
        });

    for (auto administrare : gAdministrareMedicamente) {
        if (administrare == *iterator) {
            gAdministrareMedicamente.remove(administrare);

            if (administrare != NULL) {
                delete administrare;
            }

            break;
        }
    }

    auto consultIterator = find_if(gConsultari.begin(), gConsultari.end(), [this](auto consult) -> bool {
        return consult->getMedic()->getId() == this->getId();
    });

    for (auto consultatie : gConsultari) {
        if (consultatie == *consultIterator) {
            gConsultari.remove(consultatie);

            if (consultatie != NULL) {
                delete consultatie;
            }

            break;
        }
    }
}

void UpdateInterface(CadruMedical* cadruMedical, bool& cmSelected);
void GenerateMenuLayout(int type) {
    string message, typeStr = "";
    try {
        typeStr = GetInstanceType(type);
        message = "Insereaza un nou " + typeStr;
    }
    catch (runtime_error except) {
        cout << except.what() << endl;
        return;
    }

    ImGui::SetCursorPos(ImVec2(22, 49));
    ImGui::Text(message.c_str());
    ImGui::PushItemWidth(260.f);
    {
        ImGui::SetCursorPos(ImVec2(22, 79));
        ImGui::TextDisabled("Numele complet");

        ImGui::SetCursorPos(ImVec2(20, 95));
        ImGui::InputText("##CMfullName", &CMName);
    }
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(200.f);
    {
        ImGui::SetCursorPos(ImVec2(292, 79));
        ImGui::TextDisabled("Departamentul cadrului");

        ImGui::SetCursorPos(ImVec2(290, 95));
        ImGui::InputText("##CMDepartament", &CMDepartament);
    }
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(200.f);
    {
        ImGui::SetCursorPos(ImVec2(502, 79));
        ImGui::TextDisabled("Ani experienta");

        ImGui::SetCursorPos(ImVec2(500, 95));
        ImGui::SliderInt("##CMWorkingYears", &CMWorkingYears, 0, 100);
    }
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(200.f);
    {
        ImGui::SetCursorPos(ImVec2(22, 129));
        ImGui::TextDisabled("Este rezident / (student)?");

        ImGui::SetCursorPos(ImVec2(20, 145));
        ImGui::Checkbox("##CMRezident", &CMRezident);
    }
    ImGui::PopItemWidth();
    if (type >= 1) {
        ImGui::PushItemWidth(200.f);
        {
            ImGui::SetCursorPos(ImVec2(22, 179));
            ImGui::TextDisabled("Specializare");

            ImGui::SetCursorPos(ImVec2(20, 195));
            ImGui::InputText("##CMSpecializare", &CMSpecializare);
        }
        ImGui::PopItemWidth();
    }
    ImGui::PushItemWidth(300.f);
    {
        ImGui::SetCursorPos(ImVec2(232, 129));
        ImGui::TextDisabled("Interval lucru (hh:mm:ss - hh:mm:ss)");

        ImGui::SetCursorPos(ImVec2(230, 145));
        ImGui::InputText("##CMWorkingInterval", &CMWorkingInterval);
    }
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(100.f);
    {
        ImGui::SetCursorPos(ImVec2(542, 129));
        ImGui::TextColored(ImVec4(255, 0, 0, 255), CMError.c_str());

        ImGui::SetCursorPos(ImVec2(540, 145));
        if (ImGui::Button("Salveaza", ImVec2(100, 25))) {
            try {
                Validator<string>::throwIfEmptyOrNull(CMName, "Numele");
                Validator<string>::throwIfEmptyOrNull(CMDepartament, "Departamentul");
                Validator<string>::throwIfTimeIntervalIncorrect(CMWorkingInterval, "Interval");
                if (type >= 1) {
                    Validator<string>::throwIfEmptyOrNull(CMSpecializare, "Specializarea");
                }

                TimeInterval workingInterval;
                (void)sscanf(CMWorkingInterval.c_str(), "%02d:%02d:%02d-%02d:%02d:%02d", &workingInterval.first.hour, &workingInterval.first.minute, &workingInterval.first.second, &workingInterval.second.hour, 
                    &workingInterval.second.minute, &workingInterval.second.second);

                switch (type) {
                    case 0: {
                        vector<pair<string, string>> insertable = {
                            {"name", CMName},
                            {"ds_hour", to_string(workingInterval.first.hour)},
                            {"df_hour", to_string(workingInterval.second.hour)},
                            {"ds_minute", to_string(workingInterval.first.minute)},
                            {"df_minute", to_string(workingInterval.second.minute)},
                            {"ds_second", to_string(workingInterval.first.second)},
                            {"df_second", to_string(workingInterval.second.second)},
                            {"departament", CMDepartament},
                            {"working_years", to_string(CMWorkingYears)},
                            {"resident", to_string(CMRezident)},
                        };
                        DB::GetInstance(handler)->insertable("asistents", insertable);

                        gCadreMedicale.push_back(new Asistent(DB::GetInstance(handler)->getLatestId("asistents"), CMName, CMDepartament, CMPacients, workingInterval, CMWorkingYears, CMRezident));
                        break;
                    }
                    case 1: {
                        vector<pair<string, string>> insertable = {
                            {"name", CMName},
                            {"ds_hour", to_string(workingInterval.first.hour)},
                            {"df_hour", to_string(workingInterval.second.hour)},
                            {"ds_minute", to_string(workingInterval.first.minute)},
                            {"df_minute", to_string(workingInterval.second.minute)},
                            {"ds_second", to_string(workingInterval.first.second)},
                            {"df_second", to_string(workingInterval.second.second)},
                            {"departament", CMDepartament},
                            {"working_years", to_string(CMWorkingYears)},
                            {"resident", to_string(CMRezident)},
                            {"specializare", CMSpecializare},
                        };
                        DB::GetInstance(handler)->insertable("medics", insertable);

                        gCadreMedicale.push_back(new Medic(DB::GetInstance(handler)->getLatestId("medics"), CMName, CMDepartament, CMPacients, workingInterval, CMWorkingYears, CMRezident, CMSpecializare));
                        break;
                    }
                    case 2: {
                        vector<pair<string, string>> insertable = {
                            {"name", CMName},
                            {"ds_hour", to_string(workingInterval.first.hour)},
                            {"df_hour", to_string(workingInterval.second.hour)},
                            {"ds_minute", to_string(workingInterval.first.minute)},
                            {"df_minute", to_string(workingInterval.second.minute)},
                            {"ds_second", to_string(workingInterval.first.second)},
                            {"df_second", to_string(workingInterval.second.second)},
                            {"departament", CMDepartament},
                            {"working_years", to_string(CMWorkingYears)},
                            {"resident", to_string(CMRezident)},
                            {"specializare", CMSpecializare},
                        };
                        DB::GetInstance(handler)->insertable("primary_medics", insertable);

                        gCadreMedicale.push_back(new MedicPrimar(DB::GetInstance(handler)->getLatestId("primary_medics"), CMName, CMDepartament, CMPacients, workingInterval, CMWorkingYears, CMRezident, CMSpecializare));
                        break;
                    }
                }
                gCadreMedicale.back()->write(cout);

                // reset fields
                ::CMRezident = false,
                ::CMName = ::CMDepartament = ::CMWorkingInterval = ::CMError = "",
                ::CMWorkingYears = 2;
                ::CMPacients = list<Pacient*>();
            }
            catch (runtime_error exceptionVar) {
                CMError = exceptionVar.what();
            }
        }
    }
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(400.f);
    {
        string listAnnouncement = "Lista " + typeStr;
        ImGui::SetCursorPos(ImVec2(22, 229));
        ImGui::TextDisabled(listAnnouncement.c_str());

        ImGui::SetCursorPos(ImVec2(20, 245));
        ImGui::ListBoxHeader("##PListCadreMedicale");
        for (CadruMedical* cadruMedical : gCadreMedicale)
        {
            switch (type) {
                case 0: { // tipul este asistenta medicala
                    Asistent* currentAssistent = dynamic_cast<Asistent*>(cadruMedical);

                    if (currentAssistent && !dynamic_cast<MedicPrimar*>(cadruMedical) && ImGui::Selectable(currentAssistent->getFullName().c_str()) && !localCMSelected) {
                        localEdit = currentAssistent;
                        localCMSelected = 1;
                    }
                    break;
                }
                case 1: { // tipul este medic specialist
                    Medic* currentMedic = dynamic_cast<Medic*>(cadruMedical);

                    if (currentMedic && !dynamic_cast<MedicPrimar*>(cadruMedical) && ImGui::Selectable(currentMedic->getFullName().c_str()) && !localCMSelected) {
                        localEdit = currentMedic;
                        localCMSelected = 2;
                    }
                    break;
                }
                case 2: { // tipul este medic primar
                    MedicPrimar* currentMedicPrimar = dynamic_cast<MedicPrimar*>(cadruMedical);

                    if (currentMedicPrimar && ImGui::Selectable(currentMedicPrimar->getFullName().c_str()) && !localCMSelected) {
                        localEdit = currentMedicPrimar;
                        localCMSelected = 3;
                    }
                    break;
                }
                default: {
                    throw bad_cast();
                    break;
                }
            }
        }
        ImGui::ListBoxFooter();
    }
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(400.f);
    {
        ImGui::SetCursorPos(ImVec2(432, 229));
        ImGui::TextDisabled("Lista pacienti ingrijiti");

        ImGui::SetCursorPos(ImVec2(430, 245));
        ImGui::ListBoxHeader("##PListPacientsOverview", ImVec2(250, 135));
        for (Pacient* pacient : gPacient) {
            if (ImGui::Selectable(pacient->getFullName().c_str(), contains(CMPacients, pacient))) {
                if (contains(CMPacients, pacient)) {
                    CMPacients.remove(pacient);
                }
                else {
                    CMPacients.push_back(pacient);
                }
            }
        }
        ImGui::ListBoxFooter();
    }
    ImGui::PopItemWidth();
}

void UpdateInterface(CadruMedical *cadruMedical, int &cmSelected) {
    DWORD window_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;

    if (!localCMSetInfo) {
        localCMRezident = cadruMedical->getRezident();
        localCMName = cadruMedical->getFullName(), localCMDepartament = cadruMedical->getDepartament(), localCMError = "";
        if (dynamic_cast<Medic*>(cadruMedical) || dynamic_cast<MedicPrimar*>(cadruMedical)) {
            localCMSpecializare = dynamic_cast<Medic*>(cadruMedical)->getSpecializare();
        }
        TimeInterval workingInterval = cadruMedical->getTimeInterval();
        sprintf(const_cast<char*>(localCMWorkingInterval.c_str()), "%02d:%02d:%02d-%02d:%02d:%02d", workingInterval.first.hour, workingInterval.first.minute, workingInterval.first.second, workingInterval.second.hour,
            workingInterval.second.minute, workingInterval.second.second);
        localCMWorkingYears = cadruMedical->getWorkingYears();
        localCMPacients = cadruMedical->getPacients();
        localCMSetInfo = true;
    }

    bool data = cmSelected != 0;

    ImGui::SetNextWindowSize(ImVec2(740, 460));
    ImGui::Begin("Medics", &data, window_flags);

    ImGui::SetCursorPos(ImVec2(22, 49));
    ImGui::Text(("Editeaza datele lui " + cadruMedical->getFullName()).c_str());
    ImGui::PushItemWidth(260.f);
    {
        ImGui::SetCursorPos(ImVec2(22, 79));
        ImGui::TextDisabled("Numele complet");

        ImGui::SetCursorPos(ImVec2(20, 95));
        ImGui::InputText("##CMfullName", &localCMName);
    }
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(200.f);
    {
        ImGui::SetCursorPos(ImVec2(292, 79));
        ImGui::TextDisabled("Departamentul cadrului");

        ImGui::SetCursorPos(ImVec2(290, 95));
        ImGui::InputText("##CMDepartament", &localCMDepartament);
    }
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(200.f);
    {
        ImGui::SetCursorPos(ImVec2(502, 79));
        ImGui::TextDisabled("Ani experienta");

        ImGui::SetCursorPos(ImVec2(500, 95));
        ImGui::SliderInt("##CMWorkingYears", &localCMWorkingYears, 0, 100);
    }
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(200.f);
    {
        ImGui::SetCursorPos(ImVec2(22, 129));
        ImGui::TextDisabled("Este rezident / (student)?");

        ImGui::SetCursorPos(ImVec2(20, 145));
        ImGui::Checkbox("##CMRezident", &localCMRezident);
    }
    ImGui::PopItemWidth();
    if (dynamic_cast<Medic*>(cadruMedical) || dynamic_cast<MedicPrimar*>(cadruMedical)) {
        ImGui::PushItemWidth(200.f);
        {
            ImGui::SetCursorPos(ImVec2(22, 179));
            ImGui::TextDisabled("Specializare");

            ImGui::SetCursorPos(ImVec2(20, 195));
            ImGui::InputText("##CMSpecializare", &localCMSpecializare);
        }
        ImGui::PopItemWidth();
    }
    ImGui::PushItemWidth(300.f);
    {
        ImGui::SetCursorPos(ImVec2(232, 129));
        ImGui::TextDisabled("Interval lucru (hh:mm:ss - hh:mm:ss)");

        ImGui::SetCursorPos(ImVec2(230, 145));
        ImGui::InputText("##CMWorkingInterval", &localCMWorkingInterval);
    }
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(100.f);
    {
        ImGui::SetCursorPos(ImVec2(542, 129));
        ImGui::TextColored(ImVec4(255, 0, 0, 255), localCMError.c_str());

        ImGui::SetCursorPos(ImVec2(540, 145));
        if (ImGui::Button("Editeaza", ImVec2(100, 25))) {
            try {
                Validator<string>::throwIfEmptyOrNull(localCMName);
                Validator<string>::throwIfEmptyOrNull(localCMDepartament);
                Validator<string>::throwIfTimeIntervalIncorrect(localCMWorkingInterval, "Interval");
                if (dynamic_cast<Medic*>(cadruMedical)) {
                    Validator<string>::throwIfEmptyOrNull(localCMSpecializare);
                }

                TimeInterval workingInterval;
                (void)sscanf(localCMWorkingInterval.c_str(), "%02d:%02d:%02d-%02d:%02d:%02d", &workingInterval.first.hour, &workingInterval.first.minute, &workingInterval.first.second, &workingInterval.second.hour,
                    &workingInterval.second.minute, &workingInterval.second.second);
                
                cadruMedical->setDepartament(localCMDepartament);
                cadruMedical->setFullName(localCMName);
                cadruMedical->setPacients(localCMPacients);
                cadruMedical->setRezident(localCMRezident);
                cadruMedical->setTimeInterval(workingInterval);
                cadruMedical->setWorkingYears(localCMWorkingYears);

                if (dynamic_cast<Medic*>(cadruMedical)) {
                    dynamic_cast<Medic*>(cadruMedical)->setSpecializare(localCMSpecializare);
                }
                localCMRezident = cadruMedical->getRezident();
                localCMName = cadruMedical->getFullName(), localCMDepartament = cadruMedical->getDepartament(), localCMError = "";
                if (dynamic_cast<Medic*>(cadruMedical) || dynamic_cast<MedicPrimar*>(cadruMedical)) {
                    localCMSpecializare = dynamic_cast<Medic*>(cadruMedical)->getSpecializare();
                }
                workingInterval = cadruMedical->getTimeInterval();
                sprintf(const_cast<char*>(localCMWorkingInterval.c_str()), "%02d:%02d:%02d-%02d:%02d:%02d", workingInterval.first.hour, workingInterval.first.minute, workingInterval.first.second, workingInterval.second.hour,
                    workingInterval.second.minute, workingInterval.second.second);
                localCMWorkingYears = cadruMedical->getWorkingYears();
                localCMPacients = cadruMedical->getPacients();
            }
            catch (runtime_error exceptionVar) {
                localCMError = exceptionVar.what();
            }
        }
    }
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(400.f);
    {
        ImGui::SetCursorPos(ImVec2(432, 229));
        ImGui::TextDisabled("Lista pacienti ingrijiti");

        ImGui::SetCursorPos(ImVec2(430, 245));
        ImGui::ListBoxHeader("##PListPacientsOverview", ImVec2(250, 135));
        for (Pacient* pacient : gPacient) {
            if (ImGui::Selectable(pacient->getFullName().c_str(), contains(localCMPacients, pacient))) {
                if (contains(localCMPacients, pacient)) {
                    localCMPacients.remove(pacient);
                }
                else {
                    localCMPacients.push_back(pacient);
                }
            }
        }
        ImGui::ListBoxFooter();
    }
    ImGui::PopItemWidth();
    if (dynamic_cast<MedicPrimar*>(cadruMedical)) {
        MedicPrimar *medicPrimar = dynamic_cast<MedicPrimar*>(cadruMedical);
        ImGui::PushItemWidth(400.f);
        {
            ImGui::SetCursorPos(ImVec2(22, 229));
            ImGui::TextDisabled("Medici rezidenti");

            ImGui::SetCursorPos(ImVec2(20, 245));
            ImGui::ListBoxHeader("##PListRezidenti");
            for (CadruMedical* cadruMedical : gCadreMedicale) {
                if (cadruMedical->getRezident() && ImGui::Selectable(cadruMedical->getFullName().c_str(), contains(medicPrimar->getResidentTeam(), cadruMedical))) {
                    list<CadruMedical*> reziTeam = medicPrimar->getResidentTeam();
                    if (contains(reziTeam, cadruMedical)) {
                        reziTeam.remove(cadruMedical);
                        medicPrimar->setResidentTeam(reziTeam);
                    }
                    else {
                        reziTeam.push_back(cadruMedical);
                        medicPrimar->setResidentTeam(reziTeam);
                    }
                }
            }
            ImGui::ListBoxFooter();
        }
        ImGui::PopItemWidth();
    }
    ImGui::PushItemWidth(100.f);
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.f, 0.f, 1.0f));
        ImGui::SetCursorPos(ImVec2(590, 387));
        if (ImGui::Button("Sterge cadru med.", ImVec2(125, 25))) {
            cSelection = NULL, currentSelection = NULL;
            for (auto deletableCadru : gCadreMedicale) {
                if (deletableCadru->getId() == cadruMedical->getId()) {
                    gCadreMedicale.remove(deletableCadru);

                    if (deletableCadru != NULL) {
                        if (dynamic_cast<MedicPrimar*>(deletableCadru)) {
                            DB::GetInstance(handler)->deleteById("primary_medics", deletableCadru->getId());
                        }
                        else if (dynamic_cast<Medic*>(deletableCadru)) {
                            DB::GetInstance(handler)->deleteById("medics", deletableCadru->getId());
                        }
                        delete deletableCadru;
                    }

                    localCMSetInfo = false;
                    cmSelected = 0;
                    break;
                }
            }
        }
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.3f, 1.f, 1.0f));
        ImGui::SetCursorPos(ImVec2(590, 417));

        if (ImGui::Button("Inchide meniul", ImVec2(125, 25))) {
            localCMSetInfo = false;
            cmSelected = 0;
        }
        ImGui::PopStyleColor();
    }
    ImGui::PopItemWidth();
    ImGui::End();
}

int main()
{
    handler = mysql_init(NULL);
    try {
        if (!mysql_real_connect(handler, "localhost", "root", "", "hospitalcpp", 0, NULL, 0))
            throw DatabaseException(mysql_error(handler));
    }
    catch (DatabaseException exception) {
        cout << exception.what() << endl;
        return mysql_errno(handler);
    }

    // Init the pacients
    for (auto pacients : DB::GetInstance(handler)->fetchAll("pacients")) {
        vector<HeartbeatRate*> stats;
        
        char* token = strtok(pacients[6], "|");
        while (token != NULL) {
            char* otherToken = strtok(token, ",");
            int insertable[3], k = 0;
            while (otherToken != NULL) {
                insertable[k++] = atoi(otherToken);
                otherToken = strtok(NULL, ",");
            }
            stats.push_back(new HeartbeatRate(insertable[0], insertable[1], insertable[2]));
            token = strtok(NULL, "|");
        }

        Pacient* pacient = new Pacient(atoi(pacients[0]), atoi(pacients[3]), atoi(pacients[5]), atoi(pacients[4]), string(pacients[1]), string(pacients[2]), stats);
        gPacient.push_back(pacient);
    }

    // Init the assistents
    for (auto asistents : DB::GetInstance(handler)->fetchAll("asistents")) {
        Asistent* asistent = new Asistent(atoi(asistents[0]), string(asistents[1]), string(asistents[8]), list<Pacient*>(), 
            { {atoi(asistents[2]), atoi(asistents[3]), atoi(asistents[4])}, {atoi(asistents[5]), atoi(asistents[6]), atoi(asistents[7])} }
        , atoi(asistents[9]), (bool)atoi(asistents[10]));
        gCadreMedicale.push_back(asistent);
    }

    for (auto medics : DB::GetInstance(handler)->fetchAll("medics")) {
        Medic* medic = new Medic(atoi(medics[0]), string(medics[1]), string(medics[8]), list<Pacient*>(),
            { {atoi(medics[2]), atoi(medics[3]), atoi(medics[4])}, {atoi(medics[5]), atoi(medics[6]), atoi(medics[7])} }
        , atoi(medics[9]), (bool)atoi(medics[10]), string(medics[11]));
        gCadreMedicale.push_back(medic);
    }

    for (auto medics : DB::GetInstance(handler)->fetchAll("primary_medics")) {
        MedicPrimar* medicprimar = new MedicPrimar(atoi(medics[0]), string(medics[1]), string(medics[8]), list<Pacient*>(),
            { {atoi(medics[2]), atoi(medics[3]), atoi(medics[4])}, {atoi(medics[5]), atoi(medics[6]), atoi(medics[7])} }
        , atoi(medics[9]), (bool)atoi(medics[10]), string(medics[11]));
        gCadreMedicale.push_back(medicprimar);
    }

    try {
        if (!glfwInit())
            throw GUIException("Can\'t initialize the GLFW Graphic Engine");
    }
    catch (GUIException exception) {
        cout << exception.what() << endl;
        return 1;
    }

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create the window instance with GL render engine
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Hospital Administration", NULL, NULL);
    try {
        if (window == NULL)
            throw GUIException("Can\'t create window for the GL Render Engine", (int*)&window);
    }
    catch (GUIException exception) {
        cout << exception.what() << endl;
        return 1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    // Imgui initialization
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    // Set the io interface for gestures
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Set the color scheme
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup the background color
    DWORD window_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!glfwWindowShouldClose(window)) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowSize(ImVec2(740, 460));
        ImGuiStyle* style = &ImGui::GetStyle();
        {
            style->WindowPadding = ImVec2(4, 4);
            style->WindowBorderSize = 0.f;

            style->FramePadding = ImVec2(8, 6);
            style->FrameRounding = 3.f;
            style->FrameBorderSize = 1.f;
        }

        ImGui::Begin("Consultatii", &showWindow, window_flags);
        {
            ImGui::SetCursorPos(ImVec2(22, 49));
            ImGui::Text("Insereaza un nou istoric al consultatiei");
            ImGui::PushItemWidth(100.f);
            {
                ImGui::SetCursorPos(ImVec2(297, 79));
                ImGui::TextDisabled("Ora cons.");

                ImGui::SetCursorPos(ImVec2(295, 95));
                ImGui::InputInt("##CTiH", &consultation.hour);
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(100.f);
            {
                ImGui::SetCursorPos(ImVec2(412, 79));
                ImGui::TextDisabled("Minut cons.");

                ImGui::SetCursorPos(ImVec2(410, 95));
                ImGui::InputInt("##CTiM", &consultation.minute);
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(100.f);
            {
                ImGui::SetCursorPos(ImVec2(527, 79));
                ImGui::TextDisabled("Secunda cons.");

                ImGui::SetCursorPos(ImVec2(525, 95));
                ImGui::InputInt("##CTiS", &consultation.second);
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(150.f);
            {
                ImGui::SetCursorPos(ImVec2(22, 129));
                ImGui::TextDisabled("Lista pacienti");

                ImGui::SetCursorPos(ImVec2(20, 145));
                ImGui::ListBoxHeader("##CListPacients");
                for (Pacient* pacient : gPacient)
                {
                    bool isUsed = (cPacient && pacient->getId() == cPacient->getId());
                    if (ImGui::Selectable(pacient->getFullName().c_str(), &isUsed)) {
                        cPacient = pacient;
                    }
                }
                ImGui::ListBoxFooter();
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(150.f);
            {
                ImGui::SetCursorPos(ImVec2(187, 129));
                ImGui::TextDisabled("Lista medici");

                ImGui::SetCursorPos(ImVec2(185, 145));
                ImGui::ListBoxHeader("##CCadreMedic");
                for (CadruMedical* cadruMedical : gCadreMedicale)
                {
                    Medic* currentMedic = dynamic_cast<Medic*>(cadruMedical);
                    bool isUsed = (currentMedic && cMedic && currentMedic->getId() == cMedic->getId());
                    if (currentMedic && ImGui::Selectable(currentMedic->getFullName().c_str(), &isUsed)) {
                        cMedic = currentMedic;
                    }
                }
                ImGui::ListBoxFooter();
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(100.f);
            {
                ImGui::SetCursorPos(ImVec2(352, 129));
                ImGui::TextColored(ImVec4(255, 0, 0, 255), cError.c_str());

                ImGui::SetCursorPos(ImVec2(350, 145));
                if (ImGui::Button("Salveaza", ImVec2(100, 25))) {
                    try {
                        Validator<int>::throwIfNotInRange(consultation.hour, 0, 23);
                        Validator<int>::throwIfNotInRange(consultation.minute, 0, 59);
                        Validator<int>::throwIfNotInRange(consultation.second, 0, 59);
                        Validator<Medic*>::throwIfNull(cMedic, "Medic");
                        Validator<Pacient*>::throwIfNull(cPacient, "Pacient");
                        
                        Consultari* consultare = new Consultari(DEBUG_ID);
                        gConsultari.push_back(consultare);

                        ::cMedic = NULL, ::cPacient = NULL;
                        ::consultation = { 0, 0, 0 };
                        ::cError = "";
                    }
                    catch (exception exceptionVar) {
                        cError = exceptionVar.what();
                    }
                }
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(690.f);
            {
                ImGui::SetCursorPos(ImVec2(22, 295));
                ImGui::TextDisabled("Lista istoric consultari");

                ImGui::SetCursorPos(ImVec2(22, 311));
                ImGui::ListBoxHeader("##CHistory");
                {
                    for (auto consultari : gConsultari) {
                        bool isUsed = (cSelection && cSelection == consultari);
                        if (ImGui::Selectable(((string)*consultari).c_str(), &isUsed)) {
                            if (cSelection == NULL) {
                                cSelection = consultari;
                            }
                            else {
                                cSelection = NULL;
                            }
                        }
                    }
                }
                ImGui::ListBoxFooter();
            }
            ImGui::PopItemWidth();
        }
        ImGui::End();
        if (cSelection) {
            ImGui::SetNextWindowSize(ImVec2(740, 175));

            bool isShown = (cSelection != NULL);
            ImGui::Begin("Consultatii Detalii", &isShown, window_flags);

            ImGui::PushItemWidth(100.f);
            {
                string sData = cSelection->getAllInformation();

                ImGui::SetCursorPos(ImVec2(22, 79));
                ImGui::TextDisabled(sData.c_str());
            }
            ImGui::PopItemWidth();

            ImGui::PushItemWidth(100.f);
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.f, 0.f, 1.0f));
                ImGui::SetCursorPos(ImVec2(590, 90));
                if (ImGui::Button("Sterge istoricul", ImVec2(125, 25))) {
                    for (auto deletableC : gConsultari) {
                        if (deletableC->getId() == cSelection->getId()) {
                            gConsultari.remove(deletableC);

                            if (deletableC != NULL) {
                                delete deletableC;
                            }

                            cSelection = NULL;
                            break;
                        }
                    }
                }
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.3f, 1.f, 1.0f));
                ImGui::SetCursorPos(ImVec2(590, 130));

                if (ImGui::Button("Inchide meniul", ImVec2(125, 25))) {
                    cSelection = NULL;
                }
                ImGui::PopStyleColor();
            }
            ImGui::PopItemWidth();
            ImGui::End();
        }
        
        ImGui::SetNextWindowSize(ImVec2(740, 460));
        ImGui::Begin("Administrare Medicamente", &showWindow, window_flags);
        {
            ImGui::SetCursorPos(ImVec2(22, 49));
            ImGui::Text("Insereaza un nou istoric al administrarilor de medicamente");

            ImGui::PushItemWidth(260.f);
            {
                ImGui::SetCursorPos(ImVec2(22, 79));
                ImGui::TextDisabled("Numele medicament");

                ImGui::SetCursorPos(ImVec2(20, 95));
                ImGui::InputText("##AMfullName", &medicamentName);

            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(100.f);
            {
                ImGui::SetCursorPos(ImVec2(297, 79));
                ImGui::TextDisabled("Ora adm.");

                ImGui::SetCursorPos(ImVec2(295, 95));
                ImGui::InputInt("##AMTiH", &administrationTime.hour);
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(100.f);
            {
                ImGui::SetCursorPos(ImVec2(412, 79));
                ImGui::TextDisabled("Minut adm.");

                ImGui::SetCursorPos(ImVec2(410, 95));
                ImGui::InputInt("##AMTiM", &administrationTime.minute);
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(100.f);
            {
                ImGui::SetCursorPos(ImVec2(527, 79));
                ImGui::TextDisabled("Secunda adm.");

                ImGui::SetCursorPos(ImVec2(525, 95));
                ImGui::InputInt("##AMTiS", &administrationTime.second);
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(150.f);
            {
                ImGui::SetCursorPos(ImVec2(22, 129));
                ImGui::TextDisabled("Lista pacienti");

                ImGui::SetCursorPos(ImVec2(20, 145));
                ImGui::ListBoxHeader("##AMListPacients");
                for (Pacient* pacient : gPacient)
                {
                    bool isUsed = (administree && pacient->getId() == administree->getId());
                    if (ImGui::Selectable(pacient->getFullName().c_str(), &isUsed)) {
                        administree = pacient;
                    }
                }
                ImGui::ListBoxFooter();
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(150.f);
            {
                ImGui::SetCursorPos(ImVec2(187, 129));
                ImGui::TextDisabled("Lista asistenti");

                ImGui::SetCursorPos(ImVec2(185, 145));
                ImGui::ListBoxHeader("##AMCadreMedic");
                for (CadruMedical* cadruMedical : gCadreMedicale)
                {   
                    Asistent* currentAsistent = dynamic_cast<Asistent*>(cadruMedical);
                    bool isUsed = (currentAsistent && administrator && currentAsistent->getId() == administrator->getId());
                    if (currentAsistent && ImGui::Selectable(currentAsistent->getFullName().c_str(), &isUsed)) {
                        administrator = currentAsistent;
                    }
                }
                ImGui::ListBoxFooter();
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(100.f);
            {
                ImGui::SetCursorPos(ImVec2(352, 129));
                ImGui::TextColored(ImVec4(255, 0, 0, 255), medicationError.c_str());

                ImGui::SetCursorPos(ImVec2(350, 145));
                if (ImGui::Button("Salveaza", ImVec2(100, 25))) {
                    try {
                        Validator<string>::throwIfEmptyOrNull(medicamentName, "Medicine name");
                        Validator<int>::throwIfNotInRange(administrationTime.hour, 0, 23);
                        Validator<int>::throwIfNotInRange(administrationTime.minute, 0, 59);
                        Validator<int>::throwIfNotInRange(administrationTime.second, 0, 59);
                        Validator<int>::throwIfNotInRange(typeOfAdministration, 0, 2);
                        Validator<Asistent*>::throwIfNull(administrator, "Asistent");
                        Validator<Pacient*>::throwIfNull(administree, "Pacient");

                        AdministrareMedicamente* administrareMedicamente = new AdministrareMedicamente(DEBUG_ID);
                        gAdministrareMedicamente.push_back(administrareMedicamente);

                        ::typeOfAdministration = 0;
                        ::administrator = NULL, ::administree = NULL;
                        ::administrationTime = { 0, 0, 0 };
                        ::medicamentName = ::medicationError = "";
                    }
                    catch (exception exceptionVar) {
                        medicationError = exceptionVar.what();
                    }
                }
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(100.f);
            {
                ImGui::SetCursorPos(ImVec2(352, 180));
                ImGui::TextDisabled("Tipul de adm. (0-oral, 1-intramus., 2-intravenos)");

                ImGui::SetCursorPos(ImVec2(350, 196));
                ImGui::InputInt("##AMToS", &typeOfAdministration);
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(690.f);
            {
                ImGui::SetCursorPos(ImVec2(22, 295));
                ImGui::TextDisabled("Lista istoric administrari");

                ImGui::SetCursorPos(ImVec2(22, 311));
                ImGui::ListBoxHeader("##AMHistory");
                {
                    for (auto administrareMedicament : gAdministrareMedicamente) {
                        bool isUsed = (currentSelection && currentSelection == administrareMedicament);
                        if (ImGui::Selectable(((string)*administrareMedicament).c_str(), isUsed)) {
                            if (currentSelection == NULL) {
                                currentSelection = administrareMedicament;
                            }
                            else {
                                currentSelection = NULL;
                            }
                        }
                    }
                }
                ImGui::ListBoxFooter();
            }
            ImGui::PopItemWidth();
        }
        ImGui::End();

        if (currentSelection) {
            ImGui::SetNextWindowSize(ImVec2(740, 175));

            bool isShown = (currentSelection != NULL);
            ImGui::Begin("Administrare Med. Detalii", &isShown, window_flags);

            ImGui::PushItemWidth(100.f); 
            {
                string sData = currentSelection->getAllInformation();

                ImGui::SetCursorPos(ImVec2(22, 79));
                ImGui::TextDisabled(sData.c_str());
            }
            ImGui::PopItemWidth();

            ImGui::PushItemWidth(100.f);
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.f, 0.f, 1.0f));
                ImGui::SetCursorPos(ImVec2(590, 90));
                if (ImGui::Button("Sterge istoricul", ImVec2(125, 25))) {
                    for (auto deletableAM : gAdministrareMedicamente) {
                        if (deletableAM->getId() == currentSelection->getId()) {
                            gAdministrareMedicamente.remove(deletableAM);

                            if (deletableAM != NULL) {
                                DB::GetInstance(handler)->deleteById("asistents", deletableAM->getId());
                                delete deletableAM;
                            }

                            currentSelection = NULL;
                            break;
                        }
                    }
                }
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.3f, 1.f, 1.0f));
                ImGui::SetCursorPos(ImVec2(590, 130));

                if (ImGui::Button("Inchide meniul", ImVec2(125, 25))) {
                    currentSelection = NULL;
                }
                ImGui::PopStyleColor();
            }
            ImGui::PopItemWidth();
            ImGui::End();
        }

        ImGui::SetNextWindowSize(ImVec2(740, 460));
        ImGui::Begin("Hospitalcpp", &showWindow, window_flags);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Vizualizare"))
            {
                if (ImGui::MenuItem("Pacienti", NULL, &showPacients)) {
                    showPacients = true;
                    localPacientSetInfo = false;
                    pacientSelectedItem = 0;
                    localCMSetInfo = false;
                    localCMSelected = 0;
                    showAssistents = showSpecialityMedics = showPrimaryMedics = false;
                }
                if (ImGui::MenuItem("Asistenti", NULL, &showAssistents)) {
                    showAssistents = true;
                    localPacientSetInfo = false;
                    pacientSelectedItem = 0;
                    showPacients = showSpecialityMedics = showPrimaryMedics = false;

                    localCMSetInfo = false;
                    localCMSelected = 0;

                    ::CMRezident = false,
                        ::CMName = ::CMDepartament = ::CMWorkingInterval = ::CMError = "",
                        ::CMWorkingYears = 2;
                    ::CMPacients = list<Pacient*>();
                }
                if (ImGui::MenuItem("Medici Specialisti", NULL, &showSpecialityMedics)) {
                    showSpecialityMedics = true;
                    localPacientSetInfo = false;
                    pacientSelectedItem = 0;
                    showPacients = showAssistents = showPrimaryMedics = false;

                    localCMSetInfo = false;
                    localCMSelected = 0;

                    ::CMRezident = false,
                        ::CMName = ::CMDepartament = ::CMWorkingInterval = ::CMError = "",
                        ::CMWorkingYears = 2;
                    ::CMPacients = list<Pacient*>();
                }
                if (ImGui::MenuItem("Medici Primari", NULL, &showPrimaryMedics)) {
                    showPrimaryMedics = true;
                    localPacientSetInfo = false;
                    pacientSelectedItem = 0;
                    showPacients = showAssistents = showSpecialityMedics = false;
                    localCMSetInfo = false;
                    localCMSelected = 0;
                    ::CMRezident = false,
                        ::CMName = ::CMDepartament = ::CMWorkingInterval = ::CMError = "",
                        ::CMWorkingYears = 2;
                    ::CMPacients = list<Pacient*>();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if (showPacients) {
            ImGui::SetCursorPos(ImVec2(22, 49));
            ImGui::Text("Insereaza un nou pacient");
            ImGui::PushItemWidth(260.f);
            {
                ImGui::SetCursorPos(ImVec2(22, 79));
                ImGui::TextDisabled("Numele complet pacient");

                ImGui::SetCursorPos(ImVec2(20, 95));
                ImGui::InputText("##PfullName", &pacientName);
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(200.f);
            {
                ImGui::SetCursorPos(ImVec2(292, 79));
                ImGui::TextDisabled("Greutate pacient (kg)");

                ImGui::SetCursorPos(ImVec2(290, 95));
                ImGui::SliderInt("##Pweight", &weight, 0, 200);
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(200.f);
            {
                ImGui::SetCursorPos(ImVec2(502, 79));
                ImGui::TextDisabled("Varsta pacient");

                ImGui::SetCursorPos(ImVec2(500, 95));
                ImGui::SliderInt("##PAge", &age, 0, 100);
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(200.f);
            {
                ImGui::SetCursorPos(ImVec2(22, 129));
                ImGui::TextDisabled("Inaltime pacient (cm)");

                ImGui::SetCursorPos(ImVec2(20, 145));
                ImGui::SliderInt("##PHeight", &height, 0, 300);
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(300.f);
            {
                ImGui::SetCursorPos(ImVec2(232, 129));
                ImGui::TextDisabled("Motivul venirii pacientului");

                ImGui::SetCursorPos(ImVec2(230, 145));
                ImGui::InputText("##PReason", &reason);
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(100.f);
            {   
                ImGui::SetCursorPos(ImVec2(542, 129));
                ImGui::TextColored(ImVec4(255, 0, 0, 255), saveError.c_str());

                ImGui::SetCursorPos(ImVec2(540, 145));
                if (ImGui::Button("Salveaza", ImVec2(100, 25))) {
                    try {
                        vector<pair<string, string>> insertable = {
                            {"name", pacientName},
                            {"reason", reason},
                            {"age", to_string(age)},
                            {"height", to_string(height)},
                            {"weight", to_string(weight)},
                            {"heartbeat_stats", "|"},
                        };
                        DB::GetInstance(handler)->insertable("pacients", insertable);

                        Pacient* pacient = new Pacient(DB::GetInstance(handler)->getLatestId("pacients"));
                        gPacient.push_back(pacient);

                        // Debug
                        pacient->write(cout);

                        // reset fields
                        ::pacientName = ::reason = "", ::saveError = "";
                        ::weight = 80, ::age = 18, ::height = 160;
                    }
                    catch (runtime_error exceptionVar) {
                        saveError = exceptionVar.what();
                    }
                }
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(400.f);
            {
                ImGui::SetCursorPos(ImVec2(22, 179));
                ImGui::TextDisabled("Lista pacienti");

                ImGui::SetCursorPos(ImVec2(20, 195));
                ImGui::ListBoxHeader("##PListPacients");
                for (Pacient *pacient : gPacient)
                {
                    string name = (string)*pacient;
                    if (ImGui::Selectable(name.c_str()) && !pacientSelectedItem) {
                        pacientSelectedItem = pacient->getId();
                    }
                }
                ImGui::ListBoxFooter();
            }
            ImGui::PopItemWidth();
        }
        else if (showAssistents) {
            try {
                GenerateMenuLayout(0);
            }
            catch (exception e) {
                cout << e.what() << endl;
            }
        }
        else if (showSpecialityMedics) {
            try {
                GenerateMenuLayout(1);
            }
            catch (exception e) {
                cout << e.what() << endl;
            }
        }
        else if (showPrimaryMedics) {
            try {
                GenerateMenuLayout(2);
            }
            catch (exception e) {
                cout << e.what() << endl;
            }
        }

        if (localCMSelected != 0 && localEdit) {
            UpdateInterface(localEdit, localCMSelected);
        }

        if (pacientSelectedItem != 0) {
            auto pacient = find_if(gPacient.begin(), gPacient.end(), [](Pacient *findablePacient) -> bool {
                return findablePacient->getId() == pacientSelectedItem;
            });
            bool localPacientOpened = true;

            if (!localPacientSetInfo) {
                localPacientSistole = 120, localPacientDiastol = 60, localPacientPulse = 80;
                localPacientWeight = (*pacient)->getWeight(), localPacientAge = (*pacient)->getAge(),
                localPacientHeight = (*pacient)->getHeight(),
                localPacientName = (*pacient)->getFullName(), localPacientReason = (*pacient)->getReason(),
                localPacientError = "", localPacientSetInfo = true; // seteaza datele cand se deschide fereastra unui pacient
            }

            ImGui::SetNextWindowSize(ImVec2(740, 460));
            ImGui::Begin("Pacient", &localPacientOpened, window_flags);

            ImGui::SetCursorPos(ImVec2(22, 49));
            ImGui::Text((string("Editeaza datele pacientului ") + (*pacient)->getFullName()).c_str());
            ImGui::PushItemWidth(260.f);
            {
                ImGui::SetCursorPos(ImVec2(22, 79));
                ImGui::TextDisabled("Numele complet pacient");

                ImGui::SetCursorPos(ImVec2(20, 95));
                ImGui::InputText("##PfullName", &localPacientName);
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(200.f);
            {
                ImGui::SetCursorPos(ImVec2(292, 79));
                ImGui::TextDisabled("Greutate pacient (kg)");

                ImGui::SetCursorPos(ImVec2(290, 95));
                ImGui::SliderInt("##Pweight", &localPacientWeight, 0, 200);
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(200.f);
            {
                ImGui::SetCursorPos(ImVec2(502, 79));
                ImGui::TextDisabled("Varsta pacient");

                ImGui::SetCursorPos(ImVec2(500, 95));
                ImGui::SliderInt("##PAge", &localPacientAge, 0, 100);
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(200.f);
            {
                ImGui::SetCursorPos(ImVec2(22, 129));
                ImGui::TextDisabled("Inaltime pacient (cm)");

                ImGui::SetCursorPos(ImVec2(20, 145));
                ImGui::SliderInt("##PHeight", &localPacientHeight, 0, 300);
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(300.f);
            {
                ImGui::SetCursorPos(ImVec2(232, 129));
                ImGui::TextDisabled("Motivul venirii pacientului");

                ImGui::SetCursorPos(ImVec2(230, 145));
                ImGui::InputText("##PReason", &localPacientReason);
            }
            ImGui::PushItemWidth(100.f);
            {
                ImGui::SetCursorPos(ImVec2(542, 129));
                ImGui::TextColored(ImVec4(255, 0, 0, 255), localPacientError.c_str());
                
                ImGui::SetCursorPos(ImVec2(540, 145));
                if (ImGui::Button("Editeaza", ImVec2(100, 25))) {
                    try {
                        (*pacient)->setName(localPacientName);
                        (*pacient)->setAge(localPacientAge);
                        (*pacient)->setWeight(localPacientWeight);
                        (*pacient)->setHeight(localPacientHeight);
                        (*pacient)->setReason(localPacientReason);

                            localPacientWeight = (*pacient)->getWeight(), localPacientAge = (*pacient)->getAge(),
                                localPacientHeight = (*pacient)->getHeight(), localPacientName = (*pacient)->getFullName(),
                                localPacientReason = (*pacient)->getReason(), localPacientError = "";
                       
                    }
                    catch (runtime_error exceptionVar) {
                        localPacientError = exceptionVar.what();
                    }
                }
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(100.f);
            {
                ImGui::SetCursorPos(ImVec2(22, 179));
                ImGui::Text("Indice masa corporala pacient:");

                ImGui::SetCursorPos(ImVec2(235, 179));
                pair<ImVec4, string> imcData = (*pacient)->computeIMCReport();
                ImGui::TextColored(imcData.first, imcData.second.c_str());
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(100.f);
            {   
                ImGui::SetCursorPos(ImVec2(22, 215));
                if ((*pacient)->getHeartbeatStats().size() && ImPlot::BeginPlot("Monitorizare tensiune pacient", ImVec2(694, 150))) {
                    vector<HeartbeatRate*> heartbeat = (*pacient)->getHeartbeatStats();

                    const char* labels[] = { "Sistola", "Diastola", "Puls" };
                    vector<double> positions;
                    vector<ImS16> data;

                    for (unsigned int i = 0; i < heartbeat.size(); i++) {
                        positions.push_back(i);
                    }

                    for (unsigned int i = 0; i < heartbeat.size() * 3; i++) {
                        if (i < heartbeat.size()) {
                            data.push_back((int)heartbeat[i]->getSistole());
                        }
                        else if (i >= heartbeat.size() && i < 2 * heartbeat.size()) {
                            data.push_back((int)heartbeat[i - heartbeat.size()]->getDiastole());
                        }
                        else {
                            data.push_back((int)heartbeat[i - 2 * heartbeat.size()]->getPulse());
                        }
                    }

                    ImPlot::SetupLegend(ImPlotLocation_East, ImPlotLegendFlags_Outside);
                    ImPlot::SetupAxes("Monitorizare", "Valori", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                    ImPlot::SetupAxisTicks(ImAxis_X1, positions.data(), heartbeat.size());
                    ImPlot::PlotBarGroups(labels, data.data(), 3, heartbeat.size(), 0.67f, 0, 0);
                    ImPlot::EndPlot();
                }
                else {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.00f), "Nu sunt intrari legate de monitorizarea tensiunii.");
                }
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(100.f);
            {
                ImGui::SetCursorPos(ImVec2(22, 371));
                ImGui::TextDisabled("Introdu sistola");

                ImGui::SetCursorPos(ImVec2(20, 387));
                ImGui::InputInt("##PLocalSistole", &localPacientSistole);

                ImGui::SetCursorPos(ImVec2(197, 371));
                ImGui::TextDisabled("Introdu diastola");

                ImGui::SetCursorPos(ImVec2(195, 387));
                ImGui::InputInt("##PLocalDiastol", &localPacientDiastol);

                ImGui::SetCursorPos(ImVec2(372, 371));
                ImGui::TextDisabled("Introdu puls");

                ImGui::SetCursorPos(ImVec2(370, 387));
                ImGui::InputInt("##PLocalPulse", &localPacientPulse);

                ImGui::SetCursorPos(ImVec2(480, 387));
                if (ImGui::Button("Int. Tens.", ImVec2(100, 25))) {
                    vector<HeartbeatRate*> auxiliary = (*pacient)->getHeartbeatStats();

                    auxiliary.push_back(new HeartbeatRate(localPacientSistole, localPacientDiastol, localPacientPulse));
                    (*pacient)->setHeartbeatStats(auxiliary);
                }
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(100.f);
            {   
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.f, 0.f, 1.0f));
                ImGui::SetCursorPos(ImVec2(590, 387));
                if (ImGui::Button("Sterge pacient", ImVec2(125, 25))) {
                    cSelection = NULL, currentSelection = NULL;
                    for (auto deletablePacient : gPacient) {
                        if (deletablePacient->getId() == (*pacient)->getId()) {
                            gPacient.remove(deletablePacient);

                            if (deletablePacient != NULL) {
                                DB::GetInstance(handler)->deleteById("pacients", deletablePacient->getId());
                                delete deletablePacient;
                            }

                            localPacientSetInfo = false;
                            pacientSelectedItem = 0;
                            break;
                        }
                    }
                }
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.3f, 1.f, 1.0f));
                ImGui::SetCursorPos(ImVec2(590, 417));

                if (ImGui::Button("Inchide meniul", ImVec2(125, 25))) {
                    localPacientSetInfo = false;
                    pacientSelectedItem = 0;
                }
                ImGui::PopStyleColor();
            }
            ImGui::PopItemWidth();
            ImGui::End();
        }
        ImGui::End();
        ImGui::Render();
        
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
