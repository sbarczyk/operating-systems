#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

// ===== STAŁE KONFIGURACYJNE =====
#define MAX_MEDICINE 6                          // Maksymalna pojemność apteczki
#define MAX_WAITING_PATIENTS 3                  // Maksymalna liczba pacjentów w poczekalni
#define MEDS_PER_CONSULT MAX_WAITING_PATIENTS   // Liczba leków zużywanych na konsultację (3)
#define LOW_MEDICINE_THRESHOLD MEDS_PER_CONSULT // Próg niskich zapasów leków (3)

// ===== ZMIENNE GLOBALNE STANU SYSTEMU =====
int medicine = MAX_MEDICINE;            // Aktualna liczba leków w apteczce
int waitingPatients = 0;                // Liczba pacjentów obecnie czekających
int treatedPatients = 0;                // Liczba pacjentów już obsłużonych
int finishedInCurrentConsultation = 0;  // Liczba pacjentów, którzy zakończyli bieżącą konsultację
int totalPatients = 0;                  // Całkowita liczba pacjentów (z argumentów)
int totalPharmacists = 0;               // Całkowita liczba farmaceutów (z argumentów)
int patientQueue[MAX_WAITING_PATIENTS]; // Kolejka ID pacjentów w poczekalni

// ===== MECHANIZMY SYNCHRONIZACJI =====
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;          // Mutex chroniący dostęp do zmiennych globalnych
pthread_cond_t cond_doctor = PTHREAD_COND_INITIALIZER;      // Zmienna warunkowa dla budzenia lekarza
pthread_cond_t cond_pharmacist = PTHREAD_COND_INITIALIZER;  // Zmienna warunkowa dla farmaceutów
pthread_cond_t *cond_patient;                               // Tablica zmiennych warunkowych dla pacjentów

/**
 * Funkcja pomocnicza do pobierania aktualnego czasu lokalnego
 * @return struktura tm z aktualnym czasem
 */
static struct tm get_local_time()
{
    time_t now = time(NULL);
    return *localtime(&now);
}

/**
 * Funkcja pomocnicza do generowania liczb losowych z zakresu [min, max]
 * @param min minimalna wartość
 * @param max maksymalna wartość
 * @return losowa liczba z podanego zakresu
 */
int randint(int min, int max)
{
    return min + rand() % (max - min + 1);
}

/**
 * WĄTEK PACJENTA
 * Symuluje zachowanie pojedynczego pacjenta w systemie
 */
void *patient_thread(void *arg)
{
    int id = *(int *)arg;
    
    // === FAZA 1: DOJŚCIE DO SZPITALA ===
    int t = randint(2, 5);  // Losowy czas dotarcia (2-5 sekund)
    struct tm tm = get_local_time();
    printf("[%02d:%02d:%02d] Pacjent(%d): Idę do szpitala, będę za %d s.\n",
           tm.tm_hour, tm.tm_min, tm.tm_sec, id, t);
    sleep(t);

    // === FAZA 2: PRÓBA WEJŚCIA DO POCZEKALNI ===
    while (1)
    {
        pthread_mutex_lock(&mutex);  // Sekcja krytyczna - sprawdzenie stanu poczekalni
        
        // Sprawdzenie czy poczekalnia jest pełna
        if (waitingPatients >= MAX_WAITING_PATIENTS)
        {
            pthread_mutex_unlock(&mutex);
            // Poczekalnia pełna - pacjent musi poczekać
            t = randint(2, 5);
            tm = get_local_time();
            printf("[%02d:%02d:%02d] Pacjent(%d): za dużo pacjentów, wracam później za %d s.\n",
                   tm.tm_hour, tm.tm_min, tm.tm_sec, id, t);
            sleep(t);  // Spacer dookoła szpitala
        }
        else
        {
            // === FAZA 3: WEJŚCIE DO POCZEKALNI ===
            patientQueue[waitingPatients++] = id;  // Dodanie pacjenta do kolejki
            tm = get_local_time();
            printf("[%02d:%02d:%02d] Pacjent(%d): czeka %d pacjentów na lekarza.\n",
                   tm.tm_hour, tm.tm_min, tm.tm_sec, id, waitingPatients);
            
            // Jeśli pacjent jest trzecim w kolejce - budzi lekarza
            if (waitingPatients == MAX_WAITING_PATIENTS)
            {
                tm = get_local_time();
                printf("[%02d:%02d:%02d] Pacjent(%d): budzę lekarza.\n",
                       tm.tm_hour, tm.tm_min, tm.tm_sec, id);
                pthread_cond_signal(&cond_doctor);  // Sygnał budzenia lekarza
            }
            
            // === FAZA 4: OCZEKIWANIE NA KONSULTACJĘ ===
            pthread_cond_wait(&cond_patient[id], &mutex);  // Czekanie na sygnał od lekarza
            
            // === FAZA 5: ZAKOŃCZENIE WIZYTY ===
            tm = get_local_time();
            printf("[%02d:%02d:%02d] Pacjent(%d): kończę wizytę.\n",
                   tm.tm_hour, tm.tm_min, tm.tm_sec, id);
            
            finishedInCurrentConsultation++;  // Zwiększenie licznika zakończonych wizyt
            
            // Jeśli wszyscy pacjenci z grupy zakończyli - powiadom lekarza
            if (finishedInCurrentConsultation == MAX_WAITING_PATIENTS)
            {
                pthread_cond_signal(&cond_doctor);
            }
            
            pthread_mutex_unlock(&mutex);
            break;  // Zakończenie pracy pacjenta
        }
    }
    return NULL;
}

/**
 * WĄTEK FARMACEUTY
 * Symuluje zachowanie farmaceuty dostarczającego leki
 */
void *pharmacist_thread(void *arg)
{
    int id = *(int *)arg;  // Pobranie ID farmaceuty
    
    // === FAZA 1: DOJŚCIE DO SZPITALA ===
    int t = randint(5, 15);  // Losowy czas dotarcia (5-15 sekund)
    struct tm tm = get_local_time();
    printf("[%02d:%02d:%02d] Farmaceuta(%d): idę do szpitala, będę za %d s.\n",
           tm.tm_hour, tm.tm_min, tm.tm_sec, id, t);
    sleep(t);

    pthread_mutex_lock(&mutex);  // Sekcja krytyczna
    
    // === FAZA 2: OCZEKIWANIE NA POTRZEBĘ UZUPEŁNIENIA APTECZKI ===
    // Farmaceuta czeka, aż apteczka będzie potrzebować uzupełnienia
    while (medicine >= LOW_MEDICINE_THRESHOLD && treatedPatients < totalPatients)
    {
        tm = get_local_time();
        printf("[%02d:%02d:%02d] Farmaceuta(%d): czekam na opróżnienie apteczki.\n",
               tm.tm_hour, tm.tm_min, tm.tm_sec, id);
        pthread_cond_wait(&cond_pharmacist, &mutex);  // Czekanie na sygnał
    }

    // Sprawdzenie czy wszyscy pacjenci zostali już obsłużeni
    if (treatedPatients >= totalPatients)
    {
        pthread_mutex_unlock(&mutex);
        return NULL;  // Zakończenie pracy farmaceuty
    }

    // === FAZA 3: BUDZENIE LEKARZA I SYGNALIZOWANIE CHĘCI DOSTAWY ===
    tm = get_local_time();
    printf("[%02d:%02d:%02d] Farmaceuta(%d): budzę lekarza.\n",
           tm.tm_hour, tm.tm_min, tm.tm_sec, id);
    pthread_cond_signal(&cond_doctor);  // Budzenie lekarza
    
    // Czekanie, aż lekarz będzie gotowy na odbiór dostaw
    while (medicine >= LOW_MEDICINE_THRESHOLD)
    {
        pthread_cond_wait(&cond_pharmacist, &mutex);
    }
    
    // === FAZA 4: DOSTAWA LEKÓW ===
    sleep(randint(1, 3));  // Symulacja czasu dostawy (1-3 sekundy)
    medicine = MAX_MEDICINE;  // Uzupełnienie apteczki do pełna
    tm = get_local_time();
    printf("[%02d:%02d:%02d] Farmaceuta(%d): dostarczam leki.\n",
           tm.tm_hour, tm.tm_min, tm.tm_sec, id);
    
    // Powiadomienie lekarza o zakończeniu dostawy
    pthread_cond_signal(&cond_doctor);
    
    pthread_mutex_unlock(&mutex);

    // === FAZA 5: ZAKOŃCZENIE PRACY ===
    tm = get_local_time();
    printf("[%02d:%02d:%02d] Farmaceuta(%d): zakończyłem dostawę.\n",
           tm.tm_hour, tm.tm_min, tm.tm_sec, id);
    return NULL;
}

/**
 * WĄTEK LEKARZA
 * Główny wątek kontrolujący logikę systemu - obsługuje pacjentów i farmaceutów
 */
void *doctor_thread(void *arg)
{
    (void)arg;
    
    // === GŁÓWNA PĘTLA PRACY LEKARZA ===
    while (treatedPatients < totalPatients)  // Praca do czasu obsłużenia wszystkich
    {
        pthread_mutex_lock(&mutex);  // Sekcja krytyczna

        // === FAZA 1: SPRAWDZANIE WARUNKÓW BUDZENIA ===
        // Lekarz śpi, dopóki nie wystąpi jeden z warunków:
        // 1. 3 pacjentów czeka I są leki (>=3)
        // 2. Leki się kończą (<3) I są jeszcze pacjenci do obsłużenia
        while (!(waitingPatients >= MAX_WAITING_PATIENTS && medicine >= MEDS_PER_CONSULT) && 
               !(medicine < LOW_MEDICINE_THRESHOLD && treatedPatients < totalPatients))
        {
            // Sprawdzenie warunku zakończenia pracy
            if (treatedPatients >= totalPatients)
            {
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
            pthread_cond_wait(&cond_doctor, &mutex);  // Czekanie na budzenie
        }

        // === FAZA 2: BUDZENIE LEKARZA ===
        struct tm tm = get_local_time();
        printf("[%02d:%02d:%02d] Lekarz: budzę się.\n",
               tm.tm_hour, tm.tm_min, tm.tm_sec);

        // === FAZA 3A: KONSULTACJA PACJENTÓW ===
        if (waitingPatients >= MAX_WAITING_PATIENTS && medicine >= MEDS_PER_CONSULT)
        {
            // Rozpoczęcie konsultacji
            printf("[%02d:%02d:%02d] Lekarz: konsultuję pacjentów %d, %d, %d.\n",
                   tm.tm_hour, tm.tm_min, tm.tm_sec, patientQueue[0], patientQueue[1], patientQueue[2]);

            sleep(randint(2, 4));  // Symulacja czasu konsultacji (2-4 sekundy)

            // Powiadomienie wszystkich pacjentów o zakończeniu konsultacji
            for (int i = 0; i < MAX_WAITING_PATIENTS; i++)
            {
                pthread_cond_signal(&cond_patient[patientQueue[i]]);
            }

            // Oczekiwanie na zakończenie wizyt przez wszystkich pacjentów
            while (finishedInCurrentConsultation < MAX_WAITING_PATIENTS)
            {
                pthread_cond_wait(&cond_doctor, &mutex);
            }

            // Aktualizacja stanu systemu po konsultacji
            treatedPatients += MAX_WAITING_PATIENTS;  // Zwiększenie liczby obsłużonych
            medicine -= MEDS_PER_CONSULT;             // Zużycie leków
            tm = get_local_time();
            printf("[%02d:%02d:%02d] Lekarz: apteczka po konsultacji = %d\n",
                   tm.tm_hour, tm.tm_min, tm.tm_sec, medicine);

            // Reset liczników dla następnej grupy
            finishedInCurrentConsultation = 0;
            waitingPatients = 0;

            // Sprawdzenie czy potrzeba uzupełnić apteczką i powiadomienie farmaceutów
            if (medicine < LOW_MEDICINE_THRESHOLD && treatedPatients < totalPatients)
            {
                pthread_cond_broadcast(&cond_pharmacist);
            }
        }
        // === FAZA 3B: ODBIÓR DOSTAW OD FARMACEUTY ===
        else if (medicine < LOW_MEDICINE_THRESHOLD && treatedPatients < totalPatients)
        {
            tm = get_local_time();
            printf("[%02d:%02d:%02d] Lekarz: przyjmuję dostawę leków.\n",
                   tm.tm_hour, tm.tm_min, tm.tm_sec);
            
            // Sygnał dla JEDNEGO farmaceuty, że może dostarczyć leki
            pthread_cond_signal(&cond_pharmacist);
            
            // Czekanie, aż farmaceuta dostarczy leki (medicine zostanie uzupełnione)
            while (medicine < MAX_MEDICINE && treatedPatients < totalPatients)
            {
                pthread_cond_wait(&cond_doctor, &mutex);
            }
        }

        // === FAZA 4: POWRÓT DO SNU ===
        if (treatedPatients < totalPatients)
        {
            tm = get_local_time();
            printf("[%02d:%02d:%02d] Lekarz: zasypiam.\n",
                   tm.tm_hour, tm.tm_min, tm.tm_sec);
        }
        
        pthread_mutex_unlock(&mutex);
    }
    return NULL;  // Zakończenie pracy lekarza
}

/**
 * FUNKCJA GŁÓWNA
 * Inicjalizuje system, tworzy wątki i zarządza ich zakończeniem
 */
int main(int argc, char *argv[])
{
    // === SPRAWDZENIE ARGUMENTÓW PROGRAMU ===
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <num_patients> <num_pharmacists>\n", argv[0]);
        return 1;
    }
    totalPatients = atoi(argv[1]);      // Liczba pacjentów z argumentu
    totalPharmacists = atoi(argv[2]);   // Liczba farmaceutów z argumentu

    srand(time(NULL));  // Inicjalizacja generatora liczb losowych

    // === INICJALIZACJA ZMIENNYCH WARUNKOWYCH DLA PACJENTÓW ===
    cond_patient = malloc(sizeof(pthread_cond_t) * (totalPatients + 1));
    for (int i = 0; i <= totalPatients; i++)
        pthread_cond_init(&cond_patient[i], NULL);

    // === TWORZENIE WĄTKU LEKARZA ===
    pthread_t doctor;
    pthread_create(&doctor, NULL, doctor_thread, NULL);

    // === TWORZENIE WĄTKÓW FARMACEUTÓW ===
    pthread_t *pharmacists = malloc(sizeof(pthread_t) * totalPharmacists);
    int *phids = malloc(sizeof(int) * totalPharmacists);
    for (int i = 0; i < totalPharmacists; i++)
    {
        phids[i] = i + 1;  // ID farmaceuty (1, 2, 3, ...)
        pthread_create(&pharmacists[i], NULL, pharmacist_thread, &phids[i]);
    }

    // === TWORZENIE WĄTKÓW PACJENTÓW ===
    pthread_t *patients = malloc(sizeof(pthread_t) * totalPatients);
    int *pids = malloc(sizeof(int) * totalPatients);
    for (int i = 0; i < totalPatients; i++)
    {
        pids[i] = i + 1;  // ID pacjenta (1, 2, 3, ...)
        pthread_create(&patients[i], NULL, patient_thread, &pids[i]);
    }

    // === OCZEKIWANIE NA ZAKOŃCZENIE WSZYSTKICH PACJENTÓW ===
    for (int i = 0; i < totalPatients; i++)
        pthread_join(patients[i], NULL);

    // === POWIADOMIENIE O ZAKOŃCZENIU SYSTEMU ===
    pthread_mutex_lock(&mutex);
    pthread_cond_broadcast(&cond_doctor);      // Budzenie lekarza
    pthread_cond_broadcast(&cond_pharmacist);  // Budzenie farmaceutów
    pthread_mutex_unlock(&mutex);

    // === OCZEKIWANIE NA ZAKOŃCZENIE POZOSTAŁYCH WĄTKÓW ===
    pthread_join(doctor, NULL);
    for (int i = 0; i < totalPharmacists; i++)
        pthread_join(pharmacists[i], NULL);

    // === SPRZĄTANIE PAMIĘCI I ZASOBÓW ===
    free(patients);
    free(pharmacists);
    free(pids);
    free(phids);
    
    // Zniszczenie zmiennych warunkowych pacjentów
    for (int i = 0; i <= totalPatients; i++)
        pthread_cond_destroy(&cond_patient[i]);
    free(cond_patient);

    // Zniszczenie mechanizmów synchronizacji
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_doctor);
    pthread_cond_destroy(&cond_pharmacist);

    return 0;
}