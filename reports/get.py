def extract_profits(file_path):
    # Open the file and read line by line
    with open(file_path, 'r') as file:
        for line in file:
            # Check if the line contains the target phrase
            if 'the best profit is =' in line:
                # Split the line to extract the profit value
                try:
                    # Split the line by the text 'the best profit is =' and extract the profit
                    profit = line.split('the best profit is =')[1].strip()
                    print(profit)
                except (IndexError, ValueError):
                    # Skip lines where parsing fails
                    continue
    
    return

# Example usage:
file_path = 'log.txt'  # Replace with the path to your file
extract_profits(file_path)

