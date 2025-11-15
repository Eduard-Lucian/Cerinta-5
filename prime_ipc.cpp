#include <iostream>
#include <vector>
#include <cmath>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <string>

using namespace std;

#define NUM_PROCESE 10
#define MAX_NUMAR 10000
#define DIM_INTERVAL (MAX_NUMAR / NUM_PROCESE)

// Funcție pentru a verifica dacă un număr este prim (Worker Logic)
void worker_find_primes(int start, int end) {
    auto is_prime = [](int n) {
        if (n <= 1) return false;
        if (n <= 3) return true;
        if (n % 2 == 0 || n % 3 == 0) return false;
        for (int i = 5; i * i <= n; i = i + 6) {
            if (n % i == 0 || n % (i + 2) == 0) return false;
        }
        return true;
    };

    // Scrie numerele prime către stdout (care este pipe-ul)
    for (int i = start; i <= end; ++i) {
        if (is_prime(i)) {
            cout << i << " ";
        }
    }
    cout << endl; // Terminarea output-ului
    exit(0); // Ieșire din procesul copil
}

// Logica Părintelui (Manager Logic)
void parent_manager(const string& executable_path) {
    vector<int> read_pipes(NUM_PROCESE);
    vector<pid_t> pids(NUM_PROCESE);

    // 1. Creează pipe-urile și procesele copil
    for (int i = 0; i < NUM_PROCESE; ++i) {
        int pipefd[2];

        if (pipe(pipefd) == -1) {
            cerr << "Eroare la crearea pipe-ului " << i << endl;
            return;
        }

        read_pipes[i] = pipefd[0];

        int start = i * DIM_INTERVAL + 1;
        int end = (i + 1) * DIM_INTERVAL;
        if (i == NUM_PROCESE - 1) end = MAX_NUMAR;

        pids[i] = fork();

        if (pids[i] < 0) {
            cerr << "Eroare la fork pentru procesul " << i << endl;
            return;
        }

        if (pids[i] == 0) {
            // PROCESUL COPIL

            close(pipefd[0]); // Închide capătul de citire

            // Redirecționează stdout (standard output) către capătul de scriere al pipe-ului
            if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                cerr << "Eroare dup2 in copil" << endl;
                exit(1);
            }
            close(pipefd[1]); // Închide capătul de scriere original

            // Argumente: [0]=calea, [1]="worker_mode", [2]=start, [3]=end
            string start_str = to_string(start);
            string end_str = to_string(end);

            // Copilul se înlocuiește cu o copie a sa, dar cu argumentul "worker_mode"
            execlp(executable_path.c_str(),
                   executable_path.c_str(),
                   "worker_mode", // Argument cheie pentru a activa logica worker
                   start_str.c_str(),
                   end_str.c_str(),
                   (char *)NULL);

            // Dacă execlp returnează, a eșuat
            cerr << "Eroare la execlp. Verificati calea: " << executable_path << endl;
            exit(1);
        } else {
            // PROCESUL PĂRINTE
            close(pipefd[1]); // Închide capătul de scriere
        }
    }

    // 2. Citește rezultatele
    cout << "\n Numerele prime gasite pana la " << MAX_NUMAR << " (Linux):\n" << endl;

    for (int i = 0; i < NUM_PROCESE; ++i) {
        // ... (Logica de citire din pipe similară cu soluția anterioară)
        char buffer[1024];
        stringstream ss;

        ssize_t bytes_read;
        while ((bytes_read = read(read_pipes[i], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            ss << buffer;
        }

        close(read_pipes[i]);
        waitpid(pids[i], NULL, 0);

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
    // Verifică dacă programul rulează ca proces "worker"
    if (argc > 1 && string(argv[1]) == "worker_mode") {
        if (argc >= 4) {
            int start = stoi(argv[2]);
            int end = stoi(argv[3]);
            worker_find_primes(start, end); // Rulează logica worker
            return 0;
        } else {
            cerr << "Mod worker apelat fara argumente valide." << endl;
            return 1;
        }
    } else {
        // Rulează logica Părintelui (Manager)
        if (argc == 0) {
             cerr << "Eroare: Numele executabilului nu este disponibil." << endl;
             return 1;
        }
        string executable_path = argv[0]; // Calea către executabilul curent
        parent_manager(executable_path);
        return 0;
    }
}
