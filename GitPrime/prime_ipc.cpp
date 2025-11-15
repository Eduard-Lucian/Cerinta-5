
#include <iostream>
#include <vector>
#include <cmath>
#include <windows.h>
#include <sstream>
#include <string>

using namespace std;

#define NUM_PROCESE 10
#define MAX_NUMAR 10000
#define DIM_INTERVAL (MAX_NUMAR / NUM_PROCESE)

// Functie helper pentru a extrage argumente dintr-un string (Windows CmdLine)
vector<string> parse_command_line(const string& cmd_line) {
    vector<string> args;
    stringstream ss(cmd_line);
    string arg;
    while (ss >> arg) {
        args.push_back(arg);
    }
    return args;
}

// Funcție pentru a verifica dacă un număr este prim (Worker Logic)
void worker_find_primes(int start, int end) {
    // (Funcția is_prime este definită ca lambda sau în altă parte,
    // pentru a menține consistența și claritatea)
    auto is_prime = [](int n) {
        if (n <= 1) return false;
        if (n <= 3) return true;
        if (n % 2 == 0 || n % 3 == 0) return false;
        for (int i = 5; i * i <= n; i = i + 6) {
            if (n % i == 0 || n % (i + 2) == 0) return false;
        }
        return true;
    };

    // Scrie numerele prime către stdout (pipe)
    for (int i = start; i <= end; ++i) {
        if (is_prime(i)) {
            cout << i << " ";
            cout.flush(); // Asigură scrierea imediată în pipe
        }
    }
    cout << endl;
    exit(0); // Ieșire din procesul copil
}

// Logica Părintelui (Manager Logic)
void parent_manager(const string& executable_path) {
    vector<HANDLE> read_handles(NUM_PROCESE); 
    vector<PROCESS_INFORMATION> pi_array(NUM_PROCESE); 

    // 1. Creează pipe-urile și procesele copil
    for (int i = 0; i < NUM_PROCESE; ++i) {
        HANDLE hReadPipe, hWritePipe;
        SECURITY_ATTRIBUTES saAttr;

        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0)) {
            cerr << "Eroare la CreatePipe " << i << ". Cod: " << GetLastError() << endl;
            return;
        }

        if (!SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0)) {
             cerr << "Eroare SetHandleInformation citire" << endl;
             return;
        }

        read_handles[i] = hReadPipe; 

        // Calculează intervalul
        int start = i * DIM_INTERVAL + 1;
        int end = (i + 1) * DIM_INTERVAL;
        if (i == NUM_PROCESE - 1) end = MAX_NUMAR; 

        // Linia de comandă: [calea_exe] worker_mode [start] [end]
        string command = executable_path + " worker_mode " + to_string(start) + " " + to_string(end);
        
        char cmdLine[1024]; // Buffer modificabil necesar pentru CreateProcess
        strncpy(cmdLine, command.c_str(), sizeof(cmdLine));
        cmdLine[sizeof(cmdLine) - 1] = '\0';

        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        si.hStdOutput = hWritePipe;
        si.dwFlags |= STARTF_USESTDHANDLES;

        ZeroMemory(&pi_array[i], sizeof(pi_array[i]));

        // Creează procesul copil (rulează același executabil)
        if (!CreateProcess(
            NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi_array[i]
        )) {
            cerr << "Eroare la CreateProcess " << i << ". Cod: " << GetLastError() << endl;
            CloseHandle(hReadPipe);
            CloseHandle(hWritePipe);
            return;
        }

        CloseHandle(hWritePipe); // Părintele închide capătul de scriere
    }

    // 2. Citește rezultatele
    cout << "\n Numerele prime gasite pana la " << MAX_NUMAR << " (Windows):\n" << endl;

    for (int i = 0; i < NUM_PROCESE; ++i) {
        // ... (Logica de citire din pipe similară cu soluția anterioară)
        char buffer[1024];
        DWORD bytes_read;
        stringstream ss;

        while (ReadFile(read_handles[i], buffer, sizeof(buffer) - 1, &bytes_read, NULL) && bytes_read != 0) {
            buffer[bytes_read] = '\0'; 
            ss << buffer;
        }

        CloseHandle(read_handles[i]); 
        WaitForSingleObject(pi_array[i].hProcess, INFINITE);
        CloseHandle(pi_array[i].hProcess);
        CloseHandle(pi_array[i].hThread);
        
        int prime;
        stringstream parser(ss.str());
        
        cout << "Procesul " << i + 1 << " a gasit: ";
        while (parser >> prime) {
            cout << prime << " ";
        }
        cout << endl;
    }
}

int main(int argc, char *argv[]) {
    string cmd_line = GetCommandLine();
    vector<string> args = parse_command_line(cmd_line);
    
    // Verifică dacă programul rulează ca proces "worker" (argumentul 2)
    if (args.size() > 1 && args[1] == "worker_mode") {
        if (args.size() >= 4) {
            int start = stoi(args[2]);
            int end = stoi(args[3]);
            worker_find_primes(start, end); // Rulează logica worker
            return 0;
        } else {
            cerr << "Mod worker apelat fara argumente valide." << endl;
            return 1;
        }
    } else {
        // Rulează logica Părintelui (Manager)
        if (args.empty()) {
             cerr << "Eroare: Numele executabilului nu este disponibil." << endl;
             return 1;
        }
        // Primul argument este calea către executabilul curent
        string executable_path = args[0]; 
        
        // În Windows, calea poate avea ghilimele, trebuie să le eliminăm
        if (executable_path.length() >= 2 && executable_path.front() == '"' && executable_path.back() == '"') {
            executable_path = executable_path.substr(1, executable_path.length() - 2);
        }

        parent_manager(executable_path);
        return 0;
    }
}