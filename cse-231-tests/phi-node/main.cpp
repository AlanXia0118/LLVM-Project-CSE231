int glob_x = 0;

void phi() {
    int y = 1;
    int z = 1;
    bool b;
    
    glob_x = b ? y : z;
    glob_x++;
}

int main() {
    phi();
    return 0;
}