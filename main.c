#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define IO_EXCEPTION 101 // ошибка ввода-вывода
#define ILLEGAL_ARGUMENT_EXCEPTION 102 // ошибка в значении констант a и b

// пути к основным файлам
#define RES_FILE_PATH "C:\\Users\\Admin\\Desktop\\KR\\result\\res.txt"
#define SRC_FILE_PATH "C:\\Users\\Admin\\Desktop\\KR\\source\\src.txt"
#define LOG_FILE_PATH "C:\\Users\\Admin\\Desktop\\KR\\log\\log.txt"
#define SIZE 3 // размер массива структур

// файл для логирования ошибокя
FILE *logger;

// структура для описания изменения угловой скорость вращения оболочки (W)
struct W {
    double *arrayOfW; // массив значений
    int sizeOfArray; // размер массива
};

// структура для описания изменения напора жидкости (H)
struct H {
    double *arrayOfH; // массив значений
    int sizeOfArray; // размер массива
};

/*структура для описания изменения мощности,
затрачиваемой на разбрызгивание жидкости осесимметричным разбрызгивателем*/
struct power {
    double p; // плотность жидкости
    double h0; // параметр для вычисления H
    double b; // константа
    double a; // константа
    double T; // период времени
    double t; // шаг в данном периоде
    double w10; // параметр для вычисления W
    struct W W; // угловая скорость вращения оболочки
    double F; // площадь одного отверстия
    double R; // радиус оболочки
    double u; // коэффициент расхода
    double S; /* площадь перфорированной поверхности разбрызгивателя,
                    отнесенная к 1 отверстию истечения; */
    double g; // ускорение свободного падения
    struct H H; // напор жидкости
};


struct W getW(struct power array); // получаем массив со значениями W на каждом шагу табуляции

struct H getH(struct power array); // получаем массив со значениями H на каждом шагу табуляции

double *getN(struct power array); // табулируем функцию

struct power *getDataFromFile(); // считываем массив структур с файла

void setDataToFile(struct power paramToTable, double *funcResult); // записываем результаты в файл

void cleanFile(char *path); // очищаем файл по заданому пути

int main() {
    cleanFile(LOG_FILE_PATH);
    cleanFile(RES_FILE_PATH);

    struct power *array = getDataFromFile(); // создаём структуру

    // в цикле проводим все вычисления и записуем результат в файл для каждой структуры
    for (int i = 0; i < SIZE; ++i) {
        array[i].W = getW(array[i]);
        array[i].H = getH(array[i]);
        setDataToFile(array[i], getN(array[i]));
    }
    return 0;
}

struct W getW(struct power array) {
    struct W newW;
    newW.sizeOfArray = (int) (array.T / array.t + 1); // получаем размерность мосива при определённом шаге(t)
    newW.arrayOfW = calloc(newW.sizeOfArray, sizeof(double)); // создаем массив для всех w при итерации
    int pos = 0; // индекс для записи в массив
    double step = array.t;
    array.t = 0;
    while (array.t <= array.T) { // создаём массив всех значий W на отрезке
        newW.arrayOfW[pos] = array.w10 * (1 + array.a * cos(2 * M_PI / array.T * array.t) );
        pos++;
        array.t += step;
    }
    return newW;
}

struct H getH(struct power array) {
    struct H newH;
    newH.sizeOfArray = (int) (array.T / array.t + 1); // получаем размерность мосива при определённом шаге(t)
    newH.arrayOfH = calloc(newH.sizeOfArray, sizeof(double)); // создаем массив для всем w при итерации
    int pos = 0; // индекс для записи в массив
    double step = array.t;
    array.t = 0;
    while (array.t <= array.T) {
        if ((array.t >= 0 && array.t <= array.T / 4) || (array.t >= array.T / 2 && array.t <= 3 * array.T / 4)) {
            newH.arrayOfH[pos] = array.h0 * (1 + array.b);
        } else {
            newH.arrayOfH[pos] = array.h0 * (1 - array.b);
        }
        array.t += step;
        pos++;
    }
    return newH;
}

double *getN(struct power array) {
    int numberOfFuncTab = array.W.sizeOfArray;
    double *arrayOfN;
    arrayOfN = calloc(numberOfFuncTab, sizeof(double));
    double N;
    for (int i = 0; i < numberOfFuncTab; ++i) {
        N = (2. / 3. * M_PI * array.p * pow(array.W.arrayOfW[i], 2) * array.F * array.u * pow(array.R, 3)) /
            (array.g * array.S) *
            sqrt(pow((pow(array.W.arrayOfW[i], 2) * pow(array.R, 2) + 2 * array.g * array.H.arrayOfH[i]), 3));
        arrayOfN[i] = N;
    }
    return arrayOfN;
}

struct power *getDataFromFile() {
    struct power *array;
    array = calloc(SIZE, sizeof(struct power));
    FILE *src;
    src = fopen(SRC_FILE_PATH, "r");
    if (src == 0) {
        // Логируем ошибку и завершаем работу
        logger = fopen(LOG_FILE_PATH, "a+");
        fprintf(logger, "IOException while reading from file\n");
        printf("Can not find the file");
        fclose(logger);
        exit(IO_EXCEPTION);
    }
    int pos = 0;
    while (fscanf(src, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf", &array[pos].p, &array[pos].h0, &array[pos].b,
                  &array[pos].a, &array[pos].T,
                  &array[pos].t, &array[pos].w10, &array[pos].S, &array[pos].u, &array[pos].g, &array[pos].F,
                  &array[pos].R) != EOF) {
        // проверяем значения констант
        if (array[pos].a > 1 || array[pos].a < 0 || array[pos].b > 1 || array[pos].b < 0) {
            logger = fopen(LOG_FILE_PATH, "a+");
            fprintf(logger, "Illegal value of const\n");
            fclose(logger);
            exit(ILLEGAL_ARGUMENT_EXCEPTION);
        }
        // выводим в консоль то, что считали с файла
        printf("Struct number %d", pos + 1);
        printf("\n%.0lf %.1lf %.1lf %.1lf %.0lf %.0lf %.0lf %.4lf %.2lf %.2lf %.6lf %.2lf\n", array[pos].p,
               array[pos].h0, array[pos].b,
               array[pos].a, array[pos].T,
               array[pos].t, array[pos].w10, array[pos].S, array[pos].u, array[pos].g, array[pos].F,
               array[pos].R);
        pos++;
    }
    return array;
}

void setDataToFile(struct power paramToTable, double *funcResult) {
    FILE *res;
    res = fopen(RES_FILE_PATH, "a+");
    fprintf(res,
            "Angular velocity of rotation of the shell versus time(W):\n"); // угловая скорость вращения оболочки в зависимости от времени
    for (int i = 0; i < paramToTable.W.sizeOfArray; ++i) {
        fprintf(res, "%.3lf ", paramToTable.W.arrayOfW[i]);
    }
    fprintf(res, "\nFluid pressure versus time(H):\n"); // напор жидкости в зависимости от времени
    for (int i = 0; i < paramToTable.H.sizeOfArray; ++i) {
        fprintf(res, "%.3lf ", paramToTable.H.arrayOfH[i]);
    }
    fprintf(res, "\nPower change graph(N):\n"); // график изменения мощности
    for (int i = 0; i < paramToTable.H.sizeOfArray; ++i) {
        fprintf(res, "%.3lf,  ", funcResult[i]);
    }
    fprintf(res, "\n\n\n");
    fclose(res);
}

void cleanFile(char *path) {
    FILE *file;
    file = fopen(path, "w");
    fprintf(file, "");
    fclose(file);
}