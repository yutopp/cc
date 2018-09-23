int main(void) {
    puts("IF");

    if (1) {
        puts("if0-1: true!");
    }

    if (0) {
        puts("if0-0: true!");
    }

    if (1) {
        puts("if1-1: true!");
    } else {
        puts("if1-1: false!");
    }

    if (0) {
        puts("if1-0: true!");
    } else {
        puts("if1-0: false!");
    }

    puts("END!");

    return 0;
}
