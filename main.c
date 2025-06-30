#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// 动态数组结构体定义
typedef struct {
    void** data;      // 数据指针
    int capacity;     // 数组容量
    int count;        // 元素数量
} DynamicArray;

// 文件信息结构体
typedef struct {
    char path[1024];   // 使用更大的缓冲区存储文件路径
    int lineCount;     // 行数
} FileInfo;

// 初始化动态数组
DynamicArray* initDynamicArray(int initialCapacity) {
    DynamicArray* array = malloc(sizeof(DynamicArray));
    if (array == NULL) {
        printf("内存分配失败！\n");
        return NULL;
    }

    array->data = malloc(initialCapacity * sizeof(void*));
    if (array->data == NULL) {
        printf("内存分配失败！\n");
        free(array);
        return NULL;
    }

    array->capacity = initialCapacity;
    array->count = 0;
    return array;
}

// 扩容动态数组
int expandArray(DynamicArray* array) {
    if (array == NULL) return 0;

    int newCapacity = array->capacity * 2;
    void** newData = realloc(array->data, newCapacity * sizeof(void*));

    if (newData == NULL) {
        printf("内存扩容失败！\n");
        return 0;
    }

    array->data = newData;
    array->capacity = newCapacity;
    printf("数组已扩容至 %d 个元素\n", newCapacity);
    return 1;
}

// 向动态数组添加元素
int addElement(DynamicArray* array, void* value) {
    if (array == NULL) return 0;

    // 检查是否需要扩容
    if (array->count >= array->capacity) {
        if (!expandArray(array)) return 0;
    }

    array->data[array->count++] = value;
    return 1;
}

// 释放动态数组内存
void freeDynamicArray(DynamicArray* array) {
    if (array != NULL) {
        free(array->data);
        free(array);
    }
}

// 计算文件行数
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

    // 如果文件不以换行符结束，但最后一行有内容，也要计数
    if (count == 0 && fseek(file, -1, SEEK_END) == 0) {
        if (fgetc(file) != '\n') {
            count = 1;
        }
    } else if (count > 0) {
        count++; // 最后一行可能没有换行符
    }

    fclose(file);
    return count;
}

// 读取文件指定行内容
int readLine(const char* filePath, int lineNumber, char* buffer, size_t bufferSize) {
    if (lineNumber < 1) {
        printf("错误：行号必须从1开始\n");
        return 0;
    }

    FILE* file = fopen(filePath, "r");
    if (file == NULL) {
        printf("无法打开文件: %s\n", filePath);
        return 0;
    }

    int currentLine = 0;
    char line[1024]; // 使用更大的行缓冲区

    while (fgets(line, sizeof(line), file) != NULL) {
        currentLine++;
        if (currentLine == lineNumber) {
            // 移除换行符
            size_t len = strlen(line);
            if (len > 0 && line[len-1] == '\n') {
                line[len-1] = '\0';
            }

            // 复制到缓冲区
            strncpy(buffer, line, bufferSize-1);
            buffer[bufferSize-1] = '\0';

            fclose(file);
            return 1;
        }
    }

    printf("错误：文件 %s 中不存在第 %d 行\n", filePath, lineNumber);
    fclose(file);
    return 0;
}

// 从文件读取所有数字并添加到动态数组
int readNumbersFromFile(DynamicArray* array, const char* filePath) {
    if (array == NULL || filePath == NULL) return 0;

    FILE* file = fopen(filePath, "r");
    if (file == NULL) {
        printf("无法打开文件: %s\n", filePath);
        return 0;
    }

    printf("正在读取文件: %s\n", filePath);

    int success = 1;
    double value;
    int count = 0;

    // 逐数字读取文件内容
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

    // 检查文件读取错误
    if (ferror(file)) {
        printf("读取文件 %s 时发生错误\n", filePath);
        success = 0;
    }

    if (count == 0) {
        printf("警告：文件 %s 不包含有效数字\n", filePath);
    }

    fclose(file);
    return success;
}

// 扫描目录获取所有txt文件
int scanDirectory(const char* dirPath, FileInfo*** files) {
    DIR* dir = opendir(dirPath);
    if (dir == NULL) {
        printf("无法打开目录: %s\n", dirPath);
        return 0;
    }

    DynamicArray* fileArray = initDynamicArray(10);
    if (fileArray == NULL) {
        closedir(dir);
        return 0;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // 过滤非txt文件
        const char* ext = strrchr(entry->d_name, '.');
        if (ext && strcmp(ext, ".txt") == 0) {
            // 构建完整文件路径
            char filePath[1024];
            snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, entry->d_name);

            // 计算文件行数
            int lineCount = countLines(filePath);
            if (lineCount >= 0) {
                // 添加文件信息
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

    // 转换动态数组为普通数组
    *files = (FileInfo**)fileArray->data;
    int fileCount = fileArray->count;

    // 释放动态数组结构体，但保留数据指针
    free(fileArray);

    return fileCount;
}

// 释放文件信息数组
void freeFileInfoArray(FileInfo** files, int count) {
    if (files && count > 0) {
        for (int i = 0; i < count; i++) {
            free(files[i]);
        }
        free(files);
    }
}

// 显示帮助信息
void showHelp() {
    printf("多文件数字统计系统使用说明:\n");
    printf("用法: program [选项]\n");
    printf("选项:\n");
    printf("  -h, --help         显示此帮助信息\n");
    printf("  -d, --directory    指定目录路径\n");
    printf("功能选择在程序运行时进行\n");
    printf("示例:\n");
    printf("  program            交互式输入目录路径和功能选择\n");
    printf("  program -d /path/to/data  使用指定目录并进行功能选择\n");
}

int main(int argc, char* argv[]) {
    char dirPath[1024] = ".";         // 默认目录为当前目录
    int targetLine = 0;               // 目标行号，0表示不指定
    char* targetFile = NULL;          // 目标文件名
    int useCommandLineDir = 0;        // 标记是否使用命令行指定的目录
    int functionChoice = 0;           // 功能选择

    // 解析命令行参数
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
                printf("错误：-d/--directory选项需要指定目录路径\n");
                return 1;
            }
        } else {
            printf("未知选项: %s\n", argv[i]);
            showHelp();
            return 1;
        }
    }

    // 如果没有在命令行指定目录，则提示用户输入
    if (!useCommandLineDir) {
        printf("请输入文件夹路径: ");
        if (fgets(dirPath, sizeof(dirPath), stdin)) {
            // 移除换行符
            size_t len = strlen(dirPath);
            if (len > 0 && dirPath[len - 1] == '\n') {
                dirPath[len - 1] = '\0';
            }
        } else {
            printf("输入错误，使用默认目录(当前目录)\n");
        }
    }

    // 检查目录是否存在
    struct stat st;
    if (stat(dirPath, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("错误：目录不存在或不是有效目录: %s\n", dirPath);
        return 1;
    }

    // 显示功能菜单并获取用户选择
    printf("\n请选择要执行的功能:\n");
    printf("1. 统计所有文件中的数字并计算总和\n");
    printf("2. 读取指定行的内容\n");
    printf("3. 读取指定文件的指定行\n");
    printf("请输入功能编号 (1-3): ");

    if (scanf("%d", &functionChoice) != 1) {
        printf("输入错误，请输入数字 1-3\n");
        return 1;
    }

    // 清除输入缓冲区中的换行符
    while (getchar() != '\n');

    // 根据用户选择获取额外参数
    if (functionChoice == 2) {
        printf("请输入要读取的行号: ");
        if (scanf("%d", &targetLine) != 1 || targetLine < 1) {
            printf("行号必须是大于等于1的整数\n");
            return 1;
        }
        while (getchar() != '\n'); // 清除缓冲区
    } else if (functionChoice == 3) {
        char tempFile[256];
        printf("请输入要读取的文件名: ");
        if (fgets(tempFile, sizeof(tempFile), stdin)) {
            // 移除换行符
            size_t len = strlen(tempFile);
            if (len > 0 && tempFile[len - 1] == '\n') {
                tempFile[len - 1] = '\0';
            }
            targetFile = strdup(tempFile);
        } else {
            printf("输入错误\n");
            return 1;
        }

        printf("请输入要读取的行号: ");
        if (scanf("%d", &targetLine) != 1 || targetLine < 1) {
            printf("行号必须是大于等于1的整数\n");
            free(targetFile);
            return 1;
        }
        while (getchar() != '\n'); // 清除缓冲区
    }

    printf("\n扫描目录: %s\n", dirPath);

    // 扫描目录获取所有txt文件
    FileInfo** files = NULL;
    int fileCount = scanDirectory(dirPath, &files);

    if (fileCount == 0) {
        printf("错误：在目录 %s 中未找到txt文件\n", dirPath);
        return 1;
    }

    printf("找到 %d 个txt文件\n", fileCount);

    // 如果指定了目标文件，检查其是否存在
    int targetFileIndex = -1;
    if (targetFile) {
        for (int i = 0; i < fileCount; i++) {
            const char* fileName = strrchr(files[i]->path, '/');
            if (!fileName) fileName = files[i]->path;
            else fileName++; // 跳过斜杠

            if (strcmp(fileName, targetFile) == 0) {
                targetFileIndex = i;
                break;
            }
        }

        if (targetFileIndex == -1) {
            printf("错误：在目录 %s 中未找到文件 %s\n", dirPath, targetFile);
            freeFileInfoArray(files, fileCount);
            free(targetFile);
            return 1;
        }
    }

    // 根据功能选择执行相应操作
    if (functionChoice == 1) {
        // 统计所有文件中的数字
        DynamicArray* numbers = initDynamicArray(5);
        if (numbers == NULL) {
            freeFileInfoArray(files, fileCount);
            if (targetFile) free(targetFile);
            return 1;
        }

        // 读取所有文件中的数字
        for (int i = 0; i < fileCount; i++) {
            readNumbersFromFile(numbers, files[i]->path);
        }

        // 检查是否读取到数据
        if (numbers->count == 0) {
            printf("错误：未读取到任何数据\n");
        } else {
            // 计算结果并输出
            int sum = 0;
            printf("\n读取结果 (%d 个数字):\n", numbers->count);
            for (int i = 0; i < numbers->count; i++) {
                int num = *(int*)numbers->data[i];
                printf("%d ", num);
                sum += num;
            }

            printf("\n\n所有数字的总和: %d\n", sum);
        }

        // 释放内存
        for (int i = 0; i < numbers->count; i++) {
            free(numbers->data[i]);
        }
        freeDynamicArray(numbers);
    } else if (functionChoice == 2) {
        // 读取所有文件的指定行
        printf("读取所有文件的第 %d 行内容:\n", targetLine);
        for (int i = 0; i < fileCount; i++) {
            char line[1024];
            if (readLine(files[i]->path, targetLine, line, sizeof(line))) {
                const char* fileName = strrchr(files[i]->path, '/');
                if (!fileName) fileName = files[i]->path;
                else fileName++; // 跳过斜杠

                printf("文件 %s 的第 %d 行: %s\n", fileName, targetLine, line);
            }
        }
    } else if (functionChoice == 3) {
        // 读取指定文件的指定行
        char line[1024];
        if (readLine(files[targetFileIndex]->path, targetLine, line, sizeof(line))) {
            printf("文件 %s 的第 %d 行内容: %s\n", targetFile, targetLine, line);
        }
    }

    // 释放文件信息数组
    freeFileInfoArray(files, fileCount);
    if (targetFile) free(targetFile);

    return 0;
}
