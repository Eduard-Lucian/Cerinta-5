# Numere Prime Concurente 

Acest proiect rezolvă problema găsirii numerelor prime până la $10.000$ utilizând **10 procese concurente** și comunicarea rezultatelor înapoi către procesul părinte prin **pipe-uri**.

---

##  Structura Proiectului

| Director/Fișier | Platformă | Descriere Scurtă | API-uri Cheie Folosite |
| :--- | :--- | :--- | :--- |
| **`GitPrime`** (Folder) | **Windows** | Implementarea nativă. Procesul părinte lansează 10 copii. | `CreateProcess`, `CreatePipe`, `ReadFile`. |
| **`prime_ipc.cpp`** (Fișier) | **Linux/Mac** | Implementarea bazată pe POSIX. Procesul părinte folosește `fork()` de 10 ori. | `fork()`, `pipe()`, `dup2()`, `execlp`. |

---

##  Concluzie

Am finalizat cu succes ambele implementări—**Windows** și **Linux/Mac**—folosind pipe-uri și 10 procese. Soluțiile sunt complete, folosesc setul corect de API-uri pentru fiecare sistem, și funcționează perfect pentru a împărți sarcina de calcul și a consolida rezultatele.
