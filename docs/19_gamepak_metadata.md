[Return to the index](./00_index.md)

## Game metadata
To use N64 game box art images and other metadata, place your files within the `sd:/menu/metadata/` folder.

### Game descriptions
Descriptions will be loaded by directories using each character (case-sensitive) of the full 4-character Game Code (as identified in the menu ROM information):  
i.e. for GoldenEye NTSC USA (NGEE), this would be `sd:/menu/metadata/N/G/E/E/description.txt`.  
i.e. for GoldenEye PAL (NGEP), this would be `sd:/menu/metadata/N/G/E/P/description.txt`.
Files should be added as a text format with the filename `description.txt` and closely follow the description of the first paragraph used on the back boxart. Given the room on the display, and speed of the menu, it is advised to keep descriptions as short as possible (under 800 characters).

### Game images

#### Supported images
Files must be in `PNG` format and use the following dimensions:
* American/European N64 box art sprites: 158x112
* Japanese N64 box art sprites: 112x158
* 64DD box art sprites: 129x112

Images will be loaded by directories using each character (case-sensitive) of the full 4-character Game Code (as identified in the menu ROM information):  
i.e. for GoldenEye NTSC USA (NGEE), this would be `sd:/menu/metadata/N/G/E/E/thumbnail.png`.  
i.e. for GoldenEye PAL (NGEP), this would be `sd:/menu/metadata/N/G/E/P/thumbnail.png`.

To improve compatibility between regions (as a fallback), you may exclude the region ID (last matched directory) for GamePaks to match with 3-character IDs instead:  
i.e. for GoldenEye, this would be `sd:/menu/metadata/N/G/E/thumbnail.png`.

**Warning**: Excluding the region ID may show a box art of the wrong region.  
**Note**: For future support, box art sprites should also include:  
- `thumbnail.png` (this will be used as the default image)
- `boxart_front.png`
- `boxart_back.png`
- `boxart_spine.png`
- `gamepak_front.png`

## Download package
As a starting point, here is a link to a box art pack, that has `boxart_front.png`:  
- [Third party box art](https://drive.google.com/file/d/1IpCmFqmGgGwKKmlRBxYObfFR9XywaC6n/view?usp=drive_link)

An upcoming link is [here](https://github.com/n64-tools/n64-flashcart-menu-metadata)
