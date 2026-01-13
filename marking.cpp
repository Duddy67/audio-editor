#include "marking.h"


unsigned int Marking::getNewMarkerId()
{
    unsigned int highestId = 0;

    for (size_t i = 0; i < markers.size(); i++) {
        highestId = markers[i]->id > highestId ? markers[i]->id : highestId;
    }

    return highestId + 1;
}

void Marking::insertMarker(int position)
{
    std::cout << "createMarker: " << position << std::endl; // For debog purpose

}
