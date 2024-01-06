function getRandomColor() {
    return Math.floor(Math.random() * 0xFFFFFF);
}
print("Modifying GameObject Through Script");
print('Initial color:', platform.color);

let randomColor = getRandomColor();
platform.color = randomColor;
print('Updated color:', platform.color);
