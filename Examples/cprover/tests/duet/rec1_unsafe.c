int x;
void loop() {
    if (x < 10) {
	x = x + 1;
	loop();
    }
}

void main() {
    x = 0;
    loop();

	assert(x <= 1); // unsafe
}
