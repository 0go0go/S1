#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// ��̬����ṹ�嶨��
typedef struct {
    void** data;      // ����ָ��
    int capacity;     // ��������
    int count;        // Ԫ������
} DynamicArray;

// �ļ���Ϣ�ṹ��
typedef struct {
    char path[1024];   // ʹ�ø���Ļ������洢�ļ�·��
    int lineCount;     // ����
} FileInfo;

// ��ʼ����̬����
DynamicArray* initDynamicArray(int initialCapacity) {
    DynamicArray* array = malloc(sizeof(DynamicArray));
    if (array == NULL) {
        printf("�ڴ����ʧ�ܣ�\n");
        return NULL;
    }

    array->data = malloc(initialCapacity * sizeof(void*));
    if (array->data == NULL) {
        printf("�ڴ����ʧ�ܣ�\n");
        free(array);
        return NULL;
    }

    array->capacity = initialCapacity;
    array->count = 0;
    return array;
}

// ���ݶ�̬����
int expandArray(DynamicArray* array) {
    if (array == NULL) return 0;

    int newCapacity = array->capacity * 2;
    void** newData = realloc(array->data, newCapacity * sizeof(void*));

    if (newData == NULL) {
        printf("�ڴ�����ʧ�ܣ�\n");
        return 0;
    }

    array->data = newData;
    array->capacity = newCapacity;
    printf("������������ %d ��Ԫ��\n", newCapacity);
    return 1;
}

// ��̬�������Ԫ��
int addElement(DynamicArray* array, void* value) {
    if (array == NULL) return 0;

    // ����Ƿ���Ҫ����
    if (array->count >= array->capacity) {
        if (!expandArray(array)) return 0;
    }

    array->data[array->count++] = value;
    return 1;
}

// �ͷŶ�̬�����ڴ�
void freeDynamicArray(DynamicArray* array) {
    if (array != NULL) {
        free(array->data);
        free(array);
    }
}

// �����ļ�����
int countLines(const char* filePath) {
    FILE* file = fopen(filePath, "r");
    if (file == NULL) {
        return -1;
    }

    int count = 0;
    char ch;

    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            count++;
        }
    }

    // ����ļ����Ի��з������������һ�������ݣ�ҲҪ����
    if (count == 0 && fseek(file, -1, SEEK_END) == 0) {
        if (fgetc(file) != '\n') {
            count = 1;
        }
    } else if (count > 0) {
        count++; // ���һ�п���û�л��з�
    }

    fclose(file);
    return count;
}

// ��ȡ�ļ�ָ��������
int readLine(const char* filePath, int lineNumber, char* buffer, size_t bufferSize) {
    if (lineNumber < 1) {
        printf("�����кű����1��ʼ\n");
        return 0;
    }

    FILE* file = fopen(filePath, "r");
    if (file == NULL) {
        printf("�޷����ļ�: %s\n", filePath);
        return 0;
    }

    int currentLine = 0;
    char line[1024]; // ʹ�ø�����л�����

    while (fgets(line, sizeof(line), file) != NULL) {
        currentLine++;
        if (currentLine == lineNumber) {
            // �Ƴ����з�
            size_t len = strlen(line);
            if (len > 0 && line[len-1] == '\n') {
                line[len-1] = '\0';
            }

            // ���Ƶ�������
            strncpy(buffer, line, bufferSize-1);
            buffer[bufferSize-1] = '\0';

            fclose(file);
            return 1;
        }
    }

    printf("�����ļ� %s �в����ڵ� %d ��\n", filePath, lineNumber);
    fclose(file);
    return 0;
}

// ���ļ���ȡ�������ֲ���ӵ���̬����
int readNumbersFromFile(DynamicArray* array, const char* filePath) {
    if (array == NULL || filePath == NULL) return 0;

    FILE* file = fopen(filePath, "r");
    if (file == NULL) {
        printf("�޷����ļ�: %s\n", filePath);
        return 0;
    }

    printf("���ڶ�ȡ�ļ�: %s\n", filePath);

    int success = 1;
    double value;
    int count = 0;

    // �����ֶ�ȡ�ļ�����
    while (fscanf(file, "%lf", &value) == 1) {
        int* num = malloc(sizeof(int));
        if (num == NULL) {
            success = 0;
            break;
        }
        *num = (int)value;
        if (!addElement(array, num)) {
            free(num);
            success = 0;
            break;
        }
        count++;
    }

    // ����ļ���ȡ����
    if (ferror(file)) {
        printf("��ȡ�ļ� %s ʱ��������\n", filePath);
        success = 0;
    }

    if (count == 0) {
        printf("���棺�ļ� %s ��������Ч����\n", filePath);
    }

    fclose(file);
    return success;
}

// ɨ��Ŀ¼��ȡ����txt�ļ�
int scanDirectory(const char* dirPath, FileInfo*** files) {
    DIR* dir = opendir(dirPath);
    if (dir == NULL) {
        printf("�޷���Ŀ¼: %s\n", dirPath);
        return 0;
    }

    DynamicArray* fileArray = initDynamicArray(10);
    if (fileArray == NULL) {
        closedir(dir);
        return 0;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // ���˷�txt�ļ�
        const char* ext = strrchr(entry->d_name, '.');
        if (ext && strcmp(ext, ".txt") == 0) {
            // ���������ļ�·��
            char filePath[1024];
            snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, entry->d_name);

            // �����ļ�����
            int lineCount = countLines(filePath);
            if (lineCount >= 0) {
                // ����ļ���Ϣ
                FileInfo* fileInfo = malloc(sizeof(FileInfo));
                if (fileInfo) {
                    strncpy(fileInfo->path, filePath, sizeof(fileInfo->path)-1);
                    fileInfo->path[sizeof(fileInfo->path)-1] = '\0';
                    fileInfo->lineCount = lineCount;
                    if (!addElement(fileArray, fileInfo)) {
                        free(fileInfo);
                    }
                }
            }
        }
    }

    closedir(dir);

    // ת����̬����Ϊ��ͨ����
    *files = (FileInfo**)fileArray->data;
    int fileCount = fileArray->count;

    // �ͷŶ�̬����ṹ�壬����������ָ��
    free(fileArray);

    return fileCount;
}

// �ͷ��ļ���Ϣ����
void freeFileInfoArray(FileInfo** files, int count) {
    if (files && count > 0) {
        for (int i = 0; i < count; i++) {
            free(files[i]);
        }
        free(files);
    }
}

// ��ʾ������Ϣ
void showHelp() {
    printf("���ļ�����ͳ��ϵͳʹ��˵��:\n");
    printf("�÷�: program [ѡ��]\n");
    printf("ѡ��:\n");
    printf("  -h, --help         ��ʾ�˰�����Ϣ\n");
    printf("  -d, --directory    ָ��Ŀ¼·��\n");
    printf("����ѡ���ڳ�������ʱ����\n");
    printf("ʾ��:\n");
    printf("  program            ����ʽ����Ŀ¼·���͹���ѡ��\n");
    printf("  program -d /path/to/data  ʹ��ָ��Ŀ¼�����й���ѡ��\n");
}

int main(int argc, char* argv[]) {
    char dirPath[1024] = ".";         // Ĭ��Ŀ¼Ϊ��ǰĿ¼
    int targetLine = 0;               // Ŀ���кţ�0��ʾ��ָ��
    char* targetFile = NULL;          // Ŀ���ļ���
    int useCommandLineDir = 0;        // ����Ƿ�ʹ��������ָ����Ŀ¼
    int functionChoice = 0;           // ����ѡ��

    // ���������в���
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            showHelp();
            return 0;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--directory") == 0) {
            if (i + 1 < argc) {
                strncpy(dirPath, argv[++i], sizeof(dirPath) - 1);
                dirPath[sizeof(dirPath) - 1] = '\0';
                useCommandLineDir = 1;
            } else {
                printf("����-d/--directoryѡ����Ҫָ��Ŀ¼·��\n");
                return 1;
            }
        } else {
            printf("δ֪ѡ��: %s\n", argv[i]);
            showHelp();
            return 1;
        }
    }

    // ���û����������ָ��Ŀ¼������ʾ�û�����
    if (!useCommandLineDir) {
        printf("�������ļ���·��: ");
        if (fgets(dirPath, sizeof(dirPath), stdin)) {
            // �Ƴ����з�
            size_t len = strlen(dirPath);
            if (len > 0 && dirPath[len - 1] == '\n') {
                dirPath[len - 1] = '\0';
            }
        } else {
            printf("�������ʹ��Ĭ��Ŀ¼(��ǰĿ¼)\n");
        }
    }

    // ���Ŀ¼�Ƿ����
    struct stat st;
    if (stat(dirPath, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("����Ŀ¼�����ڻ�����ЧĿ¼: %s\n", dirPath);
        return 1;
    }

    // ��ʾ���ܲ˵�����ȡ�û�ѡ��
    printf("\n��ѡ��Ҫִ�еĹ���:\n");
    printf("1. ͳ�������ļ��е����ֲ������ܺ�\n");
    printf("2. ��ȡָ���е�����\n");
    printf("3. ��ȡָ���ļ���ָ����\n");
    printf("�����빦�ܱ�� (1-3): ");

    if (scanf("%d", &functionChoice) != 1) {
        printf("����������������� 1-3\n");
        return 1;
    }

    // ������뻺�����еĻ��з�
    while (getchar() != '\n');

    // �����û�ѡ���ȡ�������
    if (functionChoice == 2) {
        printf("������Ҫ��ȡ���к�: ");
        if (scanf("%d", &targetLine) != 1 || targetLine < 1) {
            printf("�кű����Ǵ��ڵ���1������\n");
            return 1;
        }
        while (getchar() != '\n'); // ���������
    } else if (functionChoice == 3) {
        char tempFile[256];
        printf("������Ҫ��ȡ���ļ���: ");
        if (fgets(tempFile, sizeof(tempFile), stdin)) {
            // �Ƴ����з�
            size_t len = strlen(tempFile);
            if (len > 0 && tempFile[len - 1] == '\n') {
                tempFile[len - 1] = '\0';
            }
            targetFile = strdup(tempFile);
        } else {
            printf("�������\n");
            return 1;
        }

        printf("������Ҫ��ȡ���к�: ");
        if (scanf("%d", &targetLine) != 1 || targetLine < 1) {
            printf("�кű����Ǵ��ڵ���1������\n");
            free(targetFile);
            return 1;
        }
        while (getchar() != '\n'); // ���������
    }

    printf("\nɨ��Ŀ¼: %s\n", dirPath);

    // ɨ��Ŀ¼��ȡ����txt�ļ�
    FileInfo** files = NULL;
    int fileCount = scanDirectory(dirPath, &files);

    if (fileCount == 0) {
        printf("������Ŀ¼ %s ��δ�ҵ�txt�ļ�\n", dirPath);
        return 1;
    }

    printf("�ҵ� %d ��txt�ļ�\n", fileCount);

    // ���ָ����Ŀ���ļ���������Ƿ����
    int targetFileIndex = -1;
    if (targetFile) {
        for (int i = 0; i < fileCount; i++) {
            const char* fileName = strrchr(files[i]->path, '/');
            if (!fileName) fileName = files[i]->path;
            else fileName++; // ����б��

            if (strcmp(fileName, targetFile) == 0) {
                targetFileIndex = i;
                break;
            }
        }

        if (targetFileIndex == -1) {
            printf("������Ŀ¼ %s ��δ�ҵ��ļ� %s\n", dirPath, targetFile);
            freeFileInfoArray(files, fileCount);
            free(targetFile);
            return 1;
        }
    }

    // ���ݹ���ѡ��ִ����Ӧ����
    if (functionChoice == 1) {
        // ͳ�������ļ��е�����
        DynamicArray* numbers = initDynamicArray(5);
        if (numbers == NULL) {
            freeFileInfoArray(files, fileCount);
            if (targetFile) free(targetFile);
            return 1;
        }

        // ��ȡ�����ļ��е�����
        for (int i = 0; i < fileCount; i++) {
            readNumbersFromFile(numbers, files[i]->path);
        }

        // ����Ƿ��ȡ������
        if (numbers->count == 0) {
            printf("����δ��ȡ���κ�����\n");
        } else {
            // �����������
            int sum = 0;
            printf("\n��ȡ��� (%d ������):\n", numbers->count);
            for (int i = 0; i < numbers->count; i++) {
                int num = *(int*)numbers->data[i];
                printf("%d ", num);
                sum += num;
            }

            printf("\n\n�������ֵ��ܺ�: %d\n", sum);
        }

        // �ͷ��ڴ�
        for (int i = 0; i < numbers->count; i++) {
            free(numbers->data[i]);
        }
        freeDynamicArray(numbers);
    } else if (functionChoice == 2) {
        // ��ȡ�����ļ���ָ����
        printf("��ȡ�����ļ��ĵ� %d ������:\n", targetLine);
        for (int i = 0; i < fileCount; i++) {
            char line[1024];
            if (readLine(files[i]->path, targetLine, line, sizeof(line))) {
                const char* fileName = strrchr(files[i]->path, '/');
                if (!fileName) fileName = files[i]->path;
                else fileName++; // ����б��

                printf("�ļ� %s �ĵ� %d ��: %s\n", fileName, targetLine, line);
            }
        }
    } else if (functionChoice == 3) {
        // ��ȡָ���ļ���ָ����
        char line[1024];
        if (readLine(files[targetFileIndex]->path, targetLine, line, sizeof(line))) {
            printf("�ļ� %s �ĵ� %d ������: %s\n", targetFile, targetLine, line);
        }
    }

    // �ͷ��ļ���Ϣ����
    freeFileInfoArray(files, fileCount);
    if (targetFile) free(targetFile);

    return 0;
}
