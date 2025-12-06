int main(int argc, char *argv[]);
void n(char param1);


/** address: 0x08048504 */
int main(int argc, char *argv[])
{
    char local0; 		// m[esp - 532]

    n(local0);
    return;
}

/** address: 0x080484c2 */
void n(char param1)
{
    __size32 eax; 		// r24

    eax = *0x8049848;
    fgets(&param1, 512, eax);
    printf(&param1);
    exit(1);
    return;
}

