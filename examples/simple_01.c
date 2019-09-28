int g(void) {
    return 42;
}

int f(void) {
    return 42;
}

int main(void) {
    return f(g()+1+g()+3) + f(g());
}
