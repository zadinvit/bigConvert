
#ifndef readEXR_c
#define readEXR_c

bool isThisAnOpenExrFile (const char fileName[]);
int read_OpenEXR_header(const char *filename, int *planes, int *nr, int *nc);
int read_OpenEXR_file(const char *filename, float ***arr, const int planes,
                        const int nr, const int nc, const int oneplane = 0);
int write_OpenEXR_file_xyz(const char *filename, float const*const*const*arr, const int planes,
                        const int nr, const int nc, const int oneplane = 0, const int asrgb = 0);
int write_OpenEXR_file_xyz_FLOAT(const char *filename, float const*const*const*arr, const int planes,
                        const int nr, const int nc, const int oneplane = 0, const int asrgb = 0);
int write_OpenEXR_file_rgb_FLOAT(const char *filename, float const*const*const*arr, const int planes,
                        const int nr, const int nc, const int oneplane = 0, const int asrgb = 0);
int write_OpenEXR_file_rgba(const char *filename, float const*const*const*arr, const int planes, 
        const int nr, const int nc, const int oneplane = 0, const int asrgb = 0);
int isFloatEXR(const char filename []);

int read_OpenEXR_file_xyz_half(const char * filename, char * arr, const int planes, const int nr, const int nc);

#endif
