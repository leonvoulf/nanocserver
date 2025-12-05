#define NJ_INTEGRAL_ARRAY_DEFINITIONS
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include "stdio.h"
#include "nanojson.h"

#define CHECK_FOLDER L"./json_check/"

static char buffer[0x200000] = {0};

typedef struct {
    char* name;
    char* language;
    char* id;
    char* bio;
    double version;
} NameLanguageBio;


NJ_DEFINE_PARSE(NameLanguageBio, name, string, language, string, id, string, bio, string, version, floating);
NJ_DEFINE_VECTOR_PARSE(NameLanguageBio);

wchar_t** iterate_dir_files(const wchar_t* path){
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE; 
    wchar_t* files = (wchar_t*)malloc(65536);
    wchar_t** file_positions_start = (wchar_t**)files;
    wchar_t** file_pos_loc = file_positions_start;
    files += 4096;
    wchar_t* cur = files;

    hFind = FindFirstFileW(path, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Error: Unable to open directory.\n");
        return NULL;
    }

    do {
        if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0) {
            continue;
        }

        if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            (*file_pos_loc) = cur;
            file_pos_loc += 1;
            wcscpy(cur, findFileData.cFileName);
            cur += wcslen(findFileData.cFileName)+1;
        }
    } while (FindNextFileW(hFind, &findFileData) != 0);

    (*file_pos_loc) = NULL;

    FindClose(hFind);
    return file_positions_start;
}

int check_json(int argc, char** argv){
    // Server* serv = ns_create_server(NULL, 3000);
    // ns_request(serv, GET, "/a", gen_handle);
    // ns_start_server(serv);

    const wchar_t** filenames = iterate_dir_files(CHECK_FOLDER"*");
    wchar_t filename[MAX_PATH] = CHECK_FOLDER; 
    size_t prefix_length = sizeof(CHECK_FOLDER)/sizeof(wchar_t);

    allocator_t j_alloc;
    n_system_allocator_init(&j_alloc);
    JsonParser parser = {.allocator=j_alloc};

    for(wchar_t** cur_filename = filenames; cur_filename != NULL && (*cur_filename) != NULL; cur_filename++){
        wchar_t* partial_filename = (*cur_filename);
        wcscat(filename, partial_filename);
        FILE* file = _wfopen(filename, L"r");
        memset(buffer, 0, sizeof(buffer));
        size_t r = fread(buffer, 1, sizeof(buffer), file);
        buffer[r] = '\0';

        int errc = -1;
        Node* parent = create_nodes_from_parent(buffer, strlen(buffer), &errc, &parser);
        if(parent == NULL) {
            wprintf(L"Failed to parse %s, errc: %d\n", partial_filename, errc);
        } else {
            wprintf(L"Successfully parsed %s\n", partial_filename);
            if(wcscmp(partial_filename, L"pass4.json") == 0){
                A_VEC(NameLanguageBio) nlb = {0};
                json_parse_nj_vector_NameLanguageBio(parent, &nlb, sizeof(NameLanguageBio), &parser);
                char out_buf[65536] = {0};
                char* current = out_buf;
                for(size_t i = 0; i < 100 && i < nlb.count; i++){
                    NameLanguageBio n = nlb.start[i];
                    current += snprintf(current, 512, "%s,%s,%s,%s,%f\n", n.name, n.language, n.id, n.bio, n.version);
                }
                FILE* f = fopen("pass4_start.csv", "w");
                fwrite(out_buf, strlen(out_buf), 1, f);
                fclose(f);
            }
            // buffer[0] = '\0';
            // r = json_output_node(parent, buffer, 4096, 2);
            // buffer[r] = '\0';
            // printf("%s", buffer);
        }
        fclose(file);
        filename[prefix_length-1] = '\0';
    }
    free(filenames);
}