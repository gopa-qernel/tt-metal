import sys
sys.path.append('tinyrv')

import tinyrv

def main():
    rv = tinyrv.sim(xlen=32)
    
    with open('sim/add.elf', 'rb') as f:
        elf_data = f.read()
    
    rv.copy_in(0, elf_data)
    
    rv.run()
    
    print(f"a2: {rv.x[12]}")

if __name__ == '__main__':
    main()