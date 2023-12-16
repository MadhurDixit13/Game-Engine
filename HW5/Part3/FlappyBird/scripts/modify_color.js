function getRandomColor() {
    return Math.floor(Math.random() * 0xFFFFFF);
}

print("Modifying GameObject Through Script");
print('Initial color:', character.color);

let randomColor = getRandomColor();
character.color = randomColor;
print('Updated color:', character.color);
