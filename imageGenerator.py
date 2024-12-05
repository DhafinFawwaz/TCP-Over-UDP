from PIL import Image
import random
import sys

def generate_image(width, height, bit_depth, output_file):
    """
    Generates an image with random colors limited by the given bit depth.
    
    :param width: Width of the image.
    :param height: Height of the image.
    :param bit_depth: Color depth in bits (1 to 24).
    :param output_file: Path to save the generated image.
    """
    if bit_depth < 1 or bit_depth > 24:
        raise ValueError("Bit depth must be between 1 and 24.")

    # Calculate the number of color levels per channel
    levels_per_channel = 2 ** (bit_depth // 3)

    # Clamp levels to a maximum of 256 (8 bits per channel)
    levels_per_channel = min(levels_per_channel, 256)

    # Calculate the step size for each level
    step_size = 256 // levels_per_channel

    # Create a blank image
    img = Image.new("RGB", (width, height))
    pixels = img.load()

    for y in range(height):
        for x in range(width):
            # Generate random colors constrained by the bit depth
            r = random.randint(0, levels_per_channel - 1) * step_size
            g = random.randint(0, levels_per_channel - 1) * step_size
            b = random.randint(0, levels_per_channel - 1) * step_size

            # Assign the color to the pixel
            pixels[x, y] = (r, g, b)

    # Save the image
    img.save(output_file)
    print(f"Image generated and saved to {output_file}")

if __name__ == "__main__":
    # Input parameters
    try:
        width = int(input("Enter the width of the image: "))
        height = int(input("Enter the height of the image: "))
        bit_depth = int(input("Enter the bit depth (1-24): "))
        output_file = input("Enter the output file name (e.g., output.png): ")

        # Generate the image
        generate_image(width, height, bit_depth, output_file)
    except ValueError as e:
        print(f"Error: {e}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

