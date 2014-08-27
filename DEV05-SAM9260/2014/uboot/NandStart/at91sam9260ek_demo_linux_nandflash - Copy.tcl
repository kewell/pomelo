set bootstrapFile	"nandflash_at91sam9260ek.bin"
set ubootFile		"u-boot-1.3.4-exp-at91sam9260ek-nandflash.bin"

## NandFlash Mapping
set bootStrapAddr	0x00000000
set ubootAddr		0x00020000

puts "-I- === Initialize the NAND access ==="
NANDFLASH::Init

puts "-I- === Erase all the NAND flash blocs and test the erasing ==="
NANDFLASH::EraseAllNandFlash

puts "-I- === Load the bootstrap: nandflash_at91sam9-ek in the first sector ==="
NANDFLASH::sendBootFile $bootstrapFile

puts "-I- === Load the u-boot in the next sectors ==="
send_file {NandFlash} "$ubootFile" $ubootAddr 0 