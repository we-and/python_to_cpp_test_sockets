#ifndef REQUEST_UTILS_H
#define REQUEST_UTILS_H


#include <tinyxml2.h>
using namespace tinyxml2;
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
//Helper function to pad or trim a string to a specified fixed length
std::string formatFixedField(const std::string& value, size_t length) {
    if (value.length() > length) {
        return value.substr(0, length);  // Trim if too long
    } else {
        return value + std::string(length - value.length(), ' ');  // Pad with spaces if too short
    }
}

/**
 * Callback function for handling data received from a CURL request.
 * 
 * This function is designed to be used with the libcurl easy interface as a write callback function.
 * It is triggered by CURL during data transfer operations, specifically when data is received from a server.
 * The function writes the received data into a provided std::string buffer. It ensures that the buffer is
 * resized to accommodate the incoming data and then copies the data into the buffer.
 * 
 * @param contents Pointer to the data received from the server. This data needs to be appended to the existing buffer.
 * @param size Size of each data element received in bytes.
 * @param nmemb Number of data elements received.
 * @param s Pointer to the std::string that will store the received data. This string is resized and filled with the data.
 * 
 * @return The total number of bytes processed in this call. If a memory allocation failure occurs (std::bad_alloc),
 *         the function returns 0, signaling to CURL that an error occurred.
 * 
 * Usage Example:
 * std::string response_data;
 * curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
 * curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response_data);
 * 
 * Notes:
 * - This function should be used carefully within a multithreaded context as it modifies the content of the passed std::string.
 * - Proper exception handling for std::bad_alloc is crucial to avoid crashes due to memory issues.
 */
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    size_t oldLength = s->size();
    try {
        s->resize(oldLength + newLength);
    } catch (std::bad_alloc& e) {
        // handle memory problem
        return 0;
    }

    std::copy((char*)contents, (char*)contents + newLength, s->begin() + oldLength);
    return size * nmemb;
}




// Helper function to parse variable-length fields
std::string parseVariableField(const std::string& data, size_t& start, int maxLength) {
    int length = std::stoi(data.substr(start, 2));  // Assuming 2-digit length indicator
    start += 2;  // Move start past the length indicator

    if (length > maxLength) {
        throw std::runtime_error("Field length exceeds maximum allowed length.");
    }

    std::string field = data.substr(start, length);
    start += length;  // Update start position to the end of the current field
    return field;
}


// Parses an ISO8583 response string into a map of fields.
std::map<int, std::string> parseXmlISO8583(const std::string& response) {
     Logger* logger = Logger::getInstance();
    logger->log( "Parse XML");
     std::map<int, std::string> fieldMap;
 XMLDocument doc;
    doc.Parse(response.c_str());

    XMLElement* isomsg = doc.FirstChildElement("isomsg");
    if (isomsg) {
        XMLElement* field = isomsg->FirstChildElement("field");
        while (field) {

           const char* id = field->Attribute("id");
            const char* value = field->Attribute("value");

             if (id != nullptr && value != nullptr) {
            int idint = std::stoi(id);  // Convert id to an integer
            fieldMap[idint] = value;    // Insert into the map using the integer key

            // Log the parsed values
             logger->log(  "Parse str"+std::string(id)+" "+std::string(value));


        }


            
            field = field->NextSiblingElement("field");
        }
    }
     return fieldMap;
}

// Parses an ISO8583 response string into a map of fields.
std::map<int, std::string> parseISO8583(const std::string& response) {
Logger* logger = Logger::getInstance();
    std::map<int, std::string> fields;
    size_t index = 0;

    // Fixed-length fields
    fields[0] = response.substr(index, 4);
    index += 4;

    // Variable-length fields
    fields[2] = parseVariableField(response, index, 20);  // Field 2 can be up to 20 characters long
    fields[4] = response.substr(index, 12);  // Fixed length
    index += 12;
    fields[10] = response.substr(index, 12);  // Fixed length
    index += 12;

    // More variable-length fields
    fields[11] = parseVariableField(response, index, 40);
    fields[12] = parseVariableField(response, index, 40);
    fields[13] = parseVariableField(response, index, 40);
    fields[14] = parseVariableField(response, index, 256);
    fields[15] = parseVariableField(response, index, 256);
    fields[16] = parseVariableField(response, index, 256);
    fields[22] = response.substr(index, 4);
    index += 4;

    fields[23] = parseVariableField(response, index, 12);
    fields[26] = parseVariableField(response, index, 20);
    fields[31] = parseVariableField(response, index, 40);
    fields[32] = response.substr(index, 12);
    index += 12;
    fields[33] = response.substr(index, 40);
    index += 40;

    // More fields including very large variable fields
    fields[35] = parseVariableField(response, index, 660);
    fields[36] = parseVariableField(response, index, 128);
    fields[49] = response.substr(index, 3);  // Assuming 3 characters for demo, adjust as needed
    index += 3;
    fields[50] = response.substr(index, 1);
    index += 1;

    fields[55] = parseVariableField(response, index, 20);
    fields[57] = parseVariableField(response, index, 128);
    fields[59] = parseVariableField(response, index, 20);
    fields[60] = response.substr(index, 1);
    index += 1;
    fields[61] = response.substr(index, 1);
    index += 1;
    fields[62] = response.substr(index, 1);
    index += 1;

    return fields;
}


// Function to modify fields 32 and 33 in an existing ISO 8583 message
std::string modifyISO8583MessageForExpiredTokenAlert(std::string isoMessage) {
    // Parse and modify the message
    std::string modifiedMessage;
    size_t index = 0;

    // Assuming fields are present and correctly formatted up to at least field 33
    // Parsing up to field 32
    modifiedMessage += isoMessage.substr(index, 85);  // Sum of lengths of fields before field 32
    index += 85;

    // Replace field 32 (length 12)
    modifiedMessage += formatFixedField("DECLINED", 12);
    index += 12;  // Skip the original content of field 32

    // Replace field 33 (length 40)
    modifiedMessage += formatFixedField("POS notauthorized to process this request", 40);
    index += 40;  // Skip the original content of field 33

    // Append the rest of the message unchanged
    modifiedMessage += isoMessage.substr(index);

    return modifiedMessage;
}



//return the payload to the requestor
void resendToRequestor(int socket, std::string data){
    Logger* logger = Logger::getInstance();
    logger->log("resendToRequestor"+ data);
    send(socket,data.c_str(),data.size(), 0);  // Send data back to client

}
void resendToRequestorLibevent(struct bufferevent *bev, const std::string& data) {
    Logger* logger = Logger::getInstance();
    logger->log("resendToRequestor: " + data);
    bufferevent_write(bev, data.c_str(), data.size());  // Send data back to client
}
// Utility function to check if field 32 contains "TOKEN EXPIRY"
bool hasResponseTokenExpiry(const std::map<int, std::string>& fields) {
    auto it = fields.find(32);
    if (it != fields.end() && it->second == "TOKEN EXPIRY") {
        return true;
    }
    return false;
}


#endif