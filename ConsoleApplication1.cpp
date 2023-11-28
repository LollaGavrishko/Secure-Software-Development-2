#include <iostream>
#include "openssl/sha.h"
#include <sstream>
#include <iomanip>
#include <array>
#include <list>
#include <chrono> // для измерения времени и выполнения операций
#include <thread>
#include <vector>
#include <atomic>
#include <algorithm>


using namespace std;


#define LENGTH 5
#define FIRST_DIGIT 'a'
#define LAST_DIGIT 'z'

atomic<bool> allWordsFound(false);
atomic<int> wordsFoundCount(0);

//zyzzx
//apple
//mmmmm
const array<const string, 2> targetHashes =
{
    {
        "3ac16d51671ad766eb7b5b9edf182a245df80159fd9c7086a39a73e8a0b0401a", //aazza
        "6ccf348f3ed65c57f5f6c55427088c252176507b5786b530e56d1c0bcbaa0256", //naaza
        //"5ef1b1016a260f0c229c5b24afe87fe24a68b4c80f6f89535b87e0ca72a08623", //aaaab
        //"1d194f061fd453fa9caaa2a8ec9310e358fb5764c38d80c3d8df4b433fd40245", //aabab
        //"4b05cf19f4343e28099e689f08a47db3c6adf2cb5b5bd8c684a17a57b8995749", //xkklv
        //"1115dd800feaacefdf481f1f9070374a2a81e27880f187396db67958b207cbad", //zyzzx
        //"3a7bd3e2360a3d29eea436fcfb7e44c735d117c42d1c1835420b6b9942dd4f1b", //apple
        //"74e1bb62f8dabb8125a58852b63bdf6eaef667cb56ac7f7cdba6d7305c50a22f" //mmmmm
    }
};

// принимает строку в качестве входных данных и возвращает ее хэш 
string sha256(const string str)
{
    // создаем массив для хранения полученного хеша
    unsigned char hash[SHA256_DIGEST_LENGTH];

    // создаем экземпляр структуры SHA256_CTX - хранит состояние алгоритма хеширования
    SHA256_CTX sha256;
    // устанавливает начальное значение хеш-функции
    SHA256_Init(&sha256);
    // обновляем результат с переданной строкой
    SHA256_Update(&sha256, str.c_str(), str.size());
    // завершает вычисление хэша и сохраняет его в массиве hash
    SHA256_Final(hash, &sha256);

    // создаем строку для хранения шестнадцатеричного представления хеша
    stringstream ss;
    // устанавливаем флаги вывода hex и ширину вывода
    ss << hex << setfill('0');
    // проходим по байтам хеша, добавляя их шестнадцатеричное представление в stringstream
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << setw(2) << static_cast<int>(hash[i]);

    // Возвращаем сформированное шестнадцатеричное представление хеша
    return ss.str();
}

// функция перебора
string bruteForce(int start, int end, int n_thread)
{
    array<const string, targetHashes.size()> hashes = targetHashes;

    // массив символов, для хранения текущего слова, сгенерированного в результате поиска 
    char current_word[LENGTH + 1] = { 0 };
    // задаем массив number первым словом - aaaaa
    for (int i = 0; i < LENGTH; ++i)
        current_word[i] = FIRST_DIGIT;
    // для завершения строки
    current_word[LENGTH] = '\0';

    // ищем и задаем слово с которого будет брутфорсить n-ый поток
    for (int i = 0; i < start; i++)
    {
        int pos = LENGTH;
        char digit = current_word[--pos];

        for (; digit == LAST_DIGIT; digit = current_word[--pos])
        {
            if (pos == 0)
                return "no match found";
            current_word[pos] = FIRST_DIGIT;
        }
        current_word[pos] = ++digit;
    }
    //cout << current_word << '\n'; 

    auto start_time = chrono::high_resolution_clock::now();

    // перебирает диапазон start, end и генерирует хэш для каждого слова 
    for (int i = start; i < end; i++)
    {
        string word(current_word);
        string hash = sha256(word);

        // проверяет совпадает ли хеш текущего word хешу из targetHash
        auto it = find(hashes.begin(), hashes.end(), hash);
        if (it != hashes.end())
        {
            auto end_time = chrono::high_resolution_clock::now();
            chrono::duration<double> elapsed = end_time - start_time;

            cout << "\nThread " << n_thread << " found match: " << word << " in " << to_string(elapsed.count()) << " seconds";

            wordsFoundCount++;
            if(wordsFoundCount == hashes.size())
                allWordsFound.store(true, memory_order_relaxed);
        }
        /*else
            cout << word << " - is not" << endl;*/

        // генераця следующего слова
        int pos = LENGTH;
        char digit = current_word[--pos];

        for (; digit == LAST_DIGIT; digit = current_word[--pos])
        {
            if (pos == 0)
                return "err";
            current_word[pos] = FIRST_DIGIT;
        }
        current_word[pos] = ++digit;

        if (allWordsFound.load(memory_order_relaxed)) 
            return "All words have been found, exiting thread";
    }
    return "";
}


int main()
{
    int num_threads;
    cout << "Enter num thread: ";
    cin >> num_threads;

    vector<thread> threads;
    threads.reserve(num_threads); // предварительное выделение памяти

    int size = pow(LAST_DIGIT - FIRST_DIGIT + 1, LENGTH); // общее кол-во элементов для перебора
    int batch_size = size / num_threads; // размер одного кусока элементов для потока

    //num_threads = min(num_threads, static_cast<int>(thread::hardware_concurrency())); // Ограничение количества потоков
    // проходимся по каждому потоку
    for (int n_thread = 0; n_thread < num_threads; n_thread++)
    {
        int start = n_thread * batch_size; // индекс начала перебора для потока n
        int end = (n_thread == num_threads - 1) ? size : (n_thread + 1) * batch_size; //конец 

        // создаем поток
        threads.emplace_back(thread(bruteForce, start, end, n_thread));
    }

    for (auto& thread : threads)
    {
        // join блокирует вызывающий поток, пока присоединяемый поток не завершится
        thread.join();
    }

    return 0;
}


