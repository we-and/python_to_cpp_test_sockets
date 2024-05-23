
#include <string>
#include <map>
#include <iostream>
#include <tinyxml2.h>
using namespace tinyxml2;

// Helper function to parse variable-length fields
std::string parseVariableField(const std::string& data, size_t& start, int maxLength) {
    std::cout << "parse " << start<<std::endl;
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
std::map<int, std::string> parseISO8583(const std::string& response) {
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

// Parses an ISO8583 response string into a map of fields.
std::map<int, std::string> parseXmlISO8583(const std::string& response) {
    std::cout << "Parse XML"<<std::endl;
 XMLDocument doc;
    doc.Parse(response.c_str());

    XMLElement* isomsg = doc.FirstChildElement("isomsg");
    if (isomsg) {
        XMLElement* field = isomsg->FirstChildElement("field");
        while (field) {
            int id;
            const char* value;

            field->QueryIntAttribute("id", &id);
            field->Attribute("value", &value);
    std::cout << "Parse "<<id<<" "<<value<<std::endl;

            std::cout << "Field " << id << ": " << value << std::endl;
            
            field = field->NextSiblingElement("field");
        }
    }
}


std::string a = R"xml(
    <isomsg>
  <field id="0" value="0610"/>
  <field id="2" value="2454-3000-0002"/>
  <field id="4" value="490.00"/>
  <field id="10" value="10.00"/>
  <field id="11" value="k_ecpay_billspayment"/>
  <field id="12" value="2020123456"/>
  <field id="14" value="e21fa6326ebbea193d658f5b8fbb6bb8a4bd143c"/>
  <field id="15" value="fc52e179fb491c85044daab53d8f07c10fbacef3"/>
  <field id="16" value="k_ecpay_billspayment"/>
  <field id="22" value="1"/>
  <field id="23" value="0297"/>
  <field id="26" value="54.88.172.141"/>
  <field id="31" value=""/>
  <field id="32" value="SUCCESS"/>
  <field id="33" value="SUCCESS"/>
  <field id="35" value="Cullinan Test ecpay"/>
  <field id="36" value="2020123456; 2500000050060: k_ecpay_billspayment"/>
  <field id="49"><![CDATA[{&quot;itemCode&quot;:&quot;2500000050893&quot;,&quot;account&quot;:&quot;2020123456&quot;,&quot;qty&quot;:&quot;1&quot;,&quot;contact&quot;:&quot;test&quot;}]]></field>
  <field id="50" value="Y"/>
  <field id="55" value="1.2.0.0"/>
  <field id="57" value="56"/>
  <field id="59" value="SALES"/>
  <field id="60" value="null"/>
  <field id="61" value="null"/>
</isomsg>
)xml";

int  main(){
     //parse and check for token expiry
    std::string isoMessage=a;
    try {
        auto parsedFields = parseXmlISO8583(isoMessage);

        for (const auto& field : parsedFields) {
            std::cout << "Field " << field.first << ": " << field.second << std::endl;
        }

     
    } catch (const std::exception& e) {
        std::cerr << "Error parsing ISO8583 message: " <<  e.what() <<"\n"<<isoMessage<< std::endl;
        return 1;
    }
    return 0;
}