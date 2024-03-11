import heapq
import os
import pickle
from node import Node
import time
chunk_size = 524288
file_to_be_archived = 'testar'
archive_name = 'compressedar'
compressed_file = 'compressedar'


def count_chars(read_text, my_dict):
    for letter in read_text:
        if my_dict.get(letter):
            my_dict[letter] += 1
        else:
            my_dict[letter] = 1


def dfs(root, path, binary_meaning):
    if not root:
        return
    if root.char is not None:
        binary_meaning[root.char] = path
    dfs(root.left, path + "0", binary_meaning)
    dfs(root.right, path + "1", binary_meaning)


def create_freq_dict():
    freq_dict = {}
    with open(file_to_be_archived, 'rb') as file:
        text = file.read(chunk_size)
        while text:
            count_chars(text, freq_dict)
            text = file.read(chunk_size)
    return freq_dict


def create_heap(freq_dict):
    my_heap = []
    for key in freq_dict:
        freq, char = freq_dict[key], key
        node = Node(freq, char)
        heapq.heappush(my_heap, node)
    return my_heap


def create_huff_tree(heap):
    while len(heap) > 1:
        first_node = heapq.heappop(heap)
        second_node = heapq.heappop(heap)

        freq_first = first_node.freq
        freq_second = second_node.freq

        new_node = Node(freq_first + freq_second, None)
        new_node.left = first_node
        new_node.right = second_node

        heapq.heappush(heap, new_node)
    return heap[0]


def create_ar_file(binary_meaning):
    fin_len = 0
    with open(file_to_be_archived, 'rb') as original_file, open(archive_name, 'wb') as compressed_file:
        pickle.dump(binary_meaning, compressed_file)
        fin_len += compressed_file.tell()
        data_chunk = original_file.read(chunk_size)

        remaining_string = ""
        while data_chunk:
            binary_string = ""
            if remaining_string:
                binary_string += remaining_string
            len_bin_string = len(binary_string)
            for byte in data_chunk:
                if len(binary_meaning[byte]) + len_bin_string < chunk_size:
                    binary_string += binary_meaning[byte]
                else:
                    remaining_string = binary_meaning[byte]
            data_chunk = original_file.read(chunk_size)
            fin_len += process_binary_string(binary_string, compressed_file, data_chunk)


def process_binary_string(binary_string, compressed_file, data_chunk):
    fin_len = 0
    for i in range(0, len(binary_string), 8):
        byte_chunk = binary_string[i:i + 8]
        byte = int(byte_chunk, 2)
        len_byte_chunk = len(byte_chunk)

        if len_byte_chunk >= 8:
            fin_len += compressed_file.write(byte.to_bytes(1, byteorder='big'))
        else:
            padding = 8 - len_byte_chunk
            byte_chunk += "0" * padding
            byte = int(byte_chunk, 2)
            compressed_file.write(byte.to_bytes(1, byteorder='big'))
            compressed_file.write(padding.to_bytes(1, byteorder='big'))
            return fin_len
    return fin_len


def decompress_file(compressed_file_name):
    with open(compressed_file_name, 'rb') as compressed_file, open('decompressed.bin', 'wb') as decompressed_file:
        compressed_file.seek(-1, 2)
        padding_size = int.from_bytes(compressed_file.read(1), byteorder='big')
        compressed_file.seek(0)
        binary_meaning = pickle.load(compressed_file)
        print(binary_meaning)
        binary_meaning_reversed = {}
        bits_without_dict_and_padding_bytes = (os.path.getsize(compressed_file_name) * 8) - (8 * compressed_file.tell()) - 8
        for key in binary_meaning:
            binary_meaning_reversed[binary_meaning[key]] = key
        curr_byte = compressed_file.read(1)
        total_length_read = 8
        bin_string = ""
        j = 0
        while curr_byte:
            curr_string = ""
            bit_index = 0
            bin_string += format(ord(curr_byte), '08b')  # will look like this: "000001100"
            print(bin_string)
            time.sleep(1)
            while bit_index < len(bin_string):
                curr_string += bin_string[bit_index]
                if binary_meaning_reversed.get(curr_string):
                    j += 1
                    print("FOUND", chr(binary_meaning_reversed[curr_string]), "with code", curr_string, "inside string", bin_string)
                    decompressed_file.write(bytes([binary_meaning_reversed[curr_string]]))
                    total_length_read += len(curr_string)
                    if total_length_read == bits_without_dict_and_padding_bytes + (8 - padding_size):
                        print("FINISHED...")
                        return
                    bin_string = bin_string[len(curr_string):]
                    bit_index = 0
                    curr_string = ""
                    continue
                bit_index += 1
            curr_byte = compressed_file.read(1)



def main():
    # print("Started")
    # freq_dict = create_freq_dict()
    # print("Created freq dictionary")
    # heap = create_heap(freq_dict)
    # print("Created heap")

    # root = create_huff_tree(heap)
    # print("Created tree")
    # binary_meaning = {}

    # dfs(root, "", binary_meaning)
    # print("Created binary meaning, starting archiving process...")
    # create_ar_file(binary_meaning)
    print("Archived, starting decompression...")
    decompress_file(compressed_file)
    print("Finished decompression. Exited")


if __name__ == '__main__':
    main()
