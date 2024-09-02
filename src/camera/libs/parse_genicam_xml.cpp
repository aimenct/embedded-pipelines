// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "parse_genicam_xml.h"

#define MAX_STR 256


void GenicamXMLParser::_processNodeXML(xmlNode*a_node,ep::Node2*node,std::string nodename){
    xmlNode *cur_node = NULL;

    for(cur_node = a_node; cur_node; cur_node = cur_node->next){
        
        /*Check if the actual Node(cur_node) is a StringReg Node*/
        if ((cur_node->type == XML_ELEMENT_NODE)&&
        (xmlStrEqual(cur_node->name,(xmlChar*)"StringReg"))){
            
            /*With prop the propertie of the Node(Name) is obtained and with svalue we obtain the value of the propertie*/
            xmlAttr* prop = cur_node->properties;
            char* value = (char*)xmlGetProp(cur_node,prop->name);
            svalue = value;

            /*Check if the value of the Node(Content of the propertie Name) is the same as the Node to look for(nodename)
            and if it is the same then.. */
            if (svalue == nodename){
                /*A string Node is created*/
                ep::StringNode* sptr = new ep::StringNode(svalue,"");
                /*A reference between the variable node and the new string node is created */
                node->addReference(EP_HASCHILD,sptr);
                /*The new String Node is stored in the nodeset*/
                nodeset.add(sptr);
                break;
            }
            
        }
        /*Check if the actual Node(cur_node) is a IntReg/Integer/Enumeration/Float/Boolean Node*/
        else if((cur_node->type == XML_ELEMENT_NODE)&&((xmlStrEqual(cur_node->name,(xmlChar*)"IntReg"))
        ||(xmlStrEqual(cur_node->name,(xmlChar*)"Integer"))||(xmlStrEqual(cur_node->name,(xmlChar*)"Enumeration"))||
        (xmlStrEqual(cur_node->name,(xmlChar*)"Float"))||(xmlStrEqual(cur_node->name,(xmlChar*)"Boolean")))){

            xmlAttr* prop = cur_node->properties;
            char* value = (char*)xmlGetProp(cur_node,prop->name);
            svalue = value;

            if (svalue == nodename){
            ep::DataNode* dptr = new ep::DataNode(svalue,0,0);
            node->addReference(EP_HASCHILD,dptr);
            nodeset.add(dptr);
            break;}
            
        }

        else if ((cur_node->type == XML_ELEMENT_NODE)&&
        (xmlStrEqual(cur_node->name,(xmlChar*)"Command"))){

            xmlAttr* prop = cur_node->properties;
            char* value = (char*)xmlGetProp(cur_node,prop->name);
            svalue = value;

            if (svalue == nodename){
            ep::CommandNode* dptr = new ep::CommandNode(svalue,NULL);
            node->addReference(EP_HASCHILD,dptr);
            nodeset.add(dptr);
            break;
        }}

        _processNodeXML(cur_node->children,node,nodename);
        
    }  
}

void GenicamXMLParser::_processNodeTree(xmlNode *a_node){
    xmlNode *cur_node = NULL;
    for(cur_node = a_node; cur_node; cur_node = cur_node->next){
        if ((cur_node->type == XML_ELEMENT_NODE)&&
        (xmlStrEqual(cur_node->name,(xmlChar*)"Group"))){
            /*If there are node groups, then we use this function to add the category nodes that not apear in the root node*/
            xmlNode*nodenxt = cur_node->children;
            while(nodenxt != NULL){
                if((nodenxt->type == XML_ELEMENT_NODE)&&
                (xmlStrEqual(nodenxt->name,(xmlChar*)"Category"))){
                char* value = (char*)xmlGetProp(nodenxt,nodenxt->properties->name);
                char* gvalue = (char*)xmlGetProp(cur_node,cur_node->properties->name);
                /*Usually the name of the group matches from a category node and is not necessary to add it to the nodeset,
                although it is necessary when the group node cointains more than one category node and therefore
                it is necessary to add these nodes to the nodeset if any*/
                    if(*gvalue != *value){
                        std::string s = value;
                        ep::ObjectNode *ptrn = new ep::ObjectNode(s);
                        this->nodeset.add(ptrn);
                        this->root->addReference(EP_HASCHILD,ptrn);
                    }
                    nodenxt = nodenxt->next;
                }
                else if((nodenxt->type == XML_ELEMENT_NODE)&&
                !(xmlStrEqual(nodenxt->name,(xmlChar*)"text"))){
                    nodenxt = NULL;
                }
                else{
                    nodenxt = nodenxt->next;
                }
                
            }
        }
        else if ((cur_node->type == XML_ELEMENT_NODE)&&
        (xmlStrEqual(cur_node->name,(xmlChar*)"Category"))&&
        !(xmlStrEqual(cur_node->name,namegroup))){
            parseObjNode(cur_node);
        }
        _processNodeTree(cur_node->children);
        
    }  

}

        
void GenicamXMLParser::_processNodeAttr(xmlNode *a_node){
    xmlNode *cur_node = NULL;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next){
        if((cur_node->type == XML_ELEMENT_NODE)&&
        (xmlStrEqual(cur_node->name,(xmlChar*)"StringReg"))){
            parseStringRegNode(cur_node);
        }
        else if((cur_node->type == XML_ELEMENT_NODE)&&((xmlStrEqual(cur_node->name,(xmlChar*)"IntReg"))
        ||(xmlStrEqual(cur_node->name,(xmlChar*)"Integer"))||(xmlStrEqual(cur_node->name,(xmlChar*)"Enumeration"))||
        (xmlStrEqual(cur_node->name,(xmlChar*)"Float"))||(xmlStrEqual(cur_node->name,(xmlChar*)"Boolean")))){
            parseIntNode(cur_node);
        }
        else if ((cur_node->type == XML_ELEMENT_NODE)&&
        (xmlStrEqual(cur_node->name,(xmlChar*)"Command"))){
            parseComNode(cur_node);
        }
        _processNodeAttr(cur_node->children);
    }

}


GenicamXMLParser::GenicamXMLParser(const char* filename){

    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;
    /*parse the file and get the DOM*/
    doc = xmlReadFile(filename,NULL,0);
    nodeset.add(root);

    if(doc == NULL){
        printf("error: could not parse file %s\n",filename);
    }
    /*Get the root element node*/
    root_element = xmlDocGetRootElement(doc);


    _processNodeTree(root_element);
    _processNodeAttr(root_element);
    
    /*Free the document*/
    xmlFreeDoc(doc);

    /*
      *Free the global varialbes that may
      *have been allocated by the parser
    */
    xmlCleanupParser();
}

GenicamXMLParser::GenicamXMLParser(ep::ArvCam* camera,ep::Nodeset2 camnodeset){

    /*parse the file and get the DOM*/
    int* size;
    const char* xml;
    xml = camera->geniCamXml((size_t*)size);
    xmlDoc *doc = xmlReadMemory(xml,*size,"/home/iago/Documentos/embedded-pipelines/probecamera.xml",NULL,0);
    parcam = camera;
    //nodeset.add(root);

    if(doc == NULL){
        printf("error: could not parse memory file");
    }
    /*Get the root element node*/
    xmlNode* root_element = xmlDocGetRootElement(doc);
    nodeset = camnodeset;
    root = new ep::ObjectNode(std::string("Root"));
    nodeset.add(root);


    _processNodeTree(root_element);
    _processNodeAttr(root_element);
    
    /*Free the document*/
    xmlFreeDoc(doc);

    /*
      *Free the global varialbes that may
      *have been allocated by the parser
    */
    xmlCleanupParser();
}

void GenicamXMLParser::parseObjNode(xmlNode *node){
    /*Save the actual node as a variable called parent*/
    xmlNode* parent = node;
    /*Get the propertie of the actual Node (In this case the propertie Name)*/
    xmlAttr* prop = node->properties;
    /*Get the value of the propertie*/
    char* value = (char*)xmlGetProp(node,prop->name);
    /*Save the variable node as the child of the actual node*/
    node = node->children;

    /*As long as the child node is not null*/
    while(node !=NULL){
        /*Get the value of the actual node*/
        char *p = (char*)xmlNodeGetContent(node);
        std::string s = p;
        
        /*If the name of the node is equal to pFeature and the parent node is Root then...*/
        if(xmlStrEqual(node->name,(xmlChar*)"pFeature")
        &&(xmlStrEqual(xmlGetProp(parent,parent->properties->name),(xmlChar*)"Root"))){

            /*Pfeature is saved as a Object Node*/
            ep::ObjectNode *ptrn = new ep::ObjectNode(s);
            root->addReference(EP_HASCHILD,ptrn);
            nodeset.add(ptrn);
        }

        /*Else if the name of the node is only equal to pFeature then...*/
        else if (xmlStrEqual(node->name,(xmlChar*)"pFeature")){
            /*A loop for is created to check all root references*/
            for (long unsigned int i = 0;i != this->root->references().size();i++){
                /*The actual reference of root is casted as a ep::Node*/
                ep::Node2* noderef = (ep::Node2*) this->root->references().at(i)->address();
                /*Get the actual name of the reference */
                std::string nodename = noderef->name();
                svalue = value;

                /*If the nodename is equal to svalue, the node is processed and his datatype and node type are assigned*/
                if(svalue == nodename){
                _processNodeXML(parent,noderef,s);
                }
                
                }
        }

        node = node->next;
    }
    return ;
}

void GenicamXMLParser::parseStringRegNode(xmlNode* node){
    /*Get the propertie of the actual Node (In this case the propertie Name)*/
    xmlAttr* prop = node->properties;
    /*Get the value of the propertie*/
    char *value = (char*)xmlGetProp(node,prop->name);
    /*Save the variable node as the child of the actual node*/
    node = node->children;
    ep::StringNode* sptr;
    std::string nodename;
    bool parsevisib = false;

    /*As long as the child node is not null*/
    while(node != NULL){
        /*If the actual node is not equal to text node and the name of the node is equal to DisplayName then...*/
        if (!(xmlStrEqual((xmlChar*)node->name,(xmlChar*)"text"))&&(xmlStrEqual((xmlChar*)node->name,(xmlChar*)"DisplayName"))){

            /*We look in all the elements of nodeset*/
            for (int i = 0;i != nodeset.length();i++){
                /*The actual nodeset element is casted as a ep::Node*/
                ep::Node2* attrref = (ep::Node2*) nodeset.get(i);
                /*Get the name of the actual element*/
                nodename = attrref->name();
                svalue = value;

                /*If svalue is equal to the name of the actual nodeset element then we save the content of the xml node 
                in the actual nodeset element (string node)*/
                if(svalue==nodename){
                    char* p = (char*)xmlNodeGetContent(node);
                    std::string s = p;
                    sptr = (ep::StringNode*) attrref;
                    sptr->setValue(s);
                    parsevisib = true;
                }
            }
        }
        else if(xmlStrEqual(node->name,(xmlChar*)"Visibility") && parsevisib){
                    xmlChar*visib = xmlNodeGetContent(node);
                    if(xmlStrEqual(visib,(xmlChar*)"Beginner")){
                        sptr->setVisibility(EP_BEGINNER);
                    }
                    else if(xmlStrEqual(visib,(xmlChar*)"Expert")){
                        sptr->setVisibility(EP_EXPERT);
                    }
                    else if(xmlStrEqual(visib,(xmlChar*)"Guru")){
                        sptr->setVisibility(EP_GURU);
                    }
                    parsevisib = false;
                }
    node = node->next;
    }
}

void GenicamXMLParser::parseIntNode(xmlNode *node){
    /*Save the actual node as a variable called parent*/
    xmlNode*parent = node;
    /*Get the propertie of the actual Node (In this case the propertie Name)*/
    xmlAttr* prop = node->properties;
    /*Get the value of the propertie*/
    char *value = (char*)xmlGetProp(node,prop->name);
    /*Save the varialbe node as the child of the actual node*/
    node = node->children;
    ep::DataNode* dnode;
    bool hasvalue = false;
    char*parentname = (char*)parent->name;
    std::string sparentname =parentname;
    char* size = NULL;
    char* sign = NULL;
    ep::type datatype;
    std::string ssign;
    ep::type accessmode = 0;
    ep::DataNode*dptr;

    /*As long as the child node is not null*/
    while(node != NULL){

        /*We look in all the elements of nodeset*/
        for(int i = 0; i!= nodeset.length();i++){
            /*The actual nodeset element is casted as a ep::Node*/
            ep::Node2* attrref = (ep::Node2*) nodeset.get(i);
            /*Get the name of the actual element*/
            std::string nodename = attrref->name();
            svalue = value;

            /*If svalue(Content of the propertie,Name) is equal to nodename then...*/
            if (svalue == nodename&&!(xmlStrEqual((xmlChar*)node->name,(xmlChar*)"text"))){
                /*Get the content of the node*/
                char* p = (char*)xmlNodeGetContent(node);
                std::string s = p;
                /*Get the name of the node*/
                char* name = (char*)node->name;
                std::string sname = name;
                ep::DataNode*dptrrank = (ep::DataNode*)nodeset.get(i);
                dnode =(ep::DataNode*)nodeset.get(i);

                if(xmlStrEqual(node->name,(xmlChar*)"pSelected")){
                    dptrrank->setRank(2);
                }
                else if(dptrrank->rank()==0){
                    dptrrank->setRank(1);
                }

                /*If node name is equal to EnumEntry then..*/
                if(xmlStrEqual((xmlChar*)node->name,(xmlChar*)"EnumEntry")){
                    /*Get the propertie of the node*/
                    xmlAttr* enumprop = node->properties;
                    /*Get the value of the propertie*/
                    auto* enumvalue = (char*)xmlGetProp(node,enumprop->name);
                    std:: string senumvalue = enumvalue;
                    /*Get the value of the propertie of the child of the actual node*/
                    xmlNode* nodec = node->children->next;
                    unsigned int *ptr_inp;
                    ep::type nodevisib;
                    while(nodec !=NULL){
                        bool notvisib = true;
                        if(xmlStrEqual(nodec->name,(xmlChar*)"Value")){
                            auto* pc = (char*)xmlNodeGetContent(nodec);
                            std::string a(pc);
                            unsigned int inp = atoi(pc);
                            ptr_inp = new unsigned int(inp);
                            newdata.push_back(*ptr_inp);
                        }
                        else if(xmlStrEqual(nodec->name,(xmlChar*)"Visibility")){
                            xmlChar*visib = xmlNodeGetContent(node);
                            if(xmlStrEqual(visib,(xmlChar*)"Beginner")){
                                nodevisib = EP_BEGINNER;
                            }
                            else if(xmlStrEqual(visib,(xmlChar*)"Expert")){
                                nodevisib = EP_EXPERT;
                            }
                            else if(xmlStrEqual(visib,(xmlChar*)"Guru")){
                                nodevisib = EP_GURU;
                            }
                            notvisib = false;
                        }
                        if(notvisib && !xmlStrEqual(nodec->name,(xmlChar*)"text")){
                            nodevisib = EP_ENUMENTRY;
                        }
                        nodec = nodec->next;

                    }
                    ep::DataNode* dptr = new ep::DataNode(senumvalue,0,ptr_inp);
                    dptr->setVisibility(nodevisib);
                    nodeset.get(i)->addReference(EP_HASENUMVALUE,dptr);
                    nodeset.add(dptr);
                    hasvalue = true;
                }

                /*If the name of the actual node is equal to Description then...*/
                if (xmlStrEqual(node->name,(xmlChar*)"Description")){
                    /*The content of the node(variable s) is saved in the varible class tooltip*/
                    nodeset.get(i)->setTooltip(s);
                }

                else if(xmlStrEqual(node->name,(xmlChar*)"Visibility")){
                    xmlChar*visib = xmlNodeGetContent(node);
                    if(xmlStrEqual(visib,(xmlChar*)"Beginner")){
                        dnode->setVisibility(EP_BEGINNER);
                    }
                    else if(xmlStrEqual(visib,(xmlChar*)"Expert")){
                        dnode->setVisibility(EP_EXPERT);
                    }
                    else if(xmlStrEqual(visib,(xmlChar*)"Guru")){
                        dnode->setVisibility(EP_GURU);
                    }
                }

                /*If the name of the actual node is equal to Length then...*/
                else if(xmlStrEqual(node->name,(xmlChar*)"Length")){
                    /*The content of the node is saves in the variable size*/
                    size =(char*)xmlNodeGetContent(node);
                }

                else if((xmlStrEqual((xmlChar*)node->name,(xmlChar*)"Unit"))){
                    ep::StringNode* sptr = new ep::StringNode(sname,s);
                    nodeset.get(i)->addReference(EP_HASCHILD,sptr);
                    nodeset.add(sptr);
                }

                /*If the name of the actual node is equal to Accessmode then...*/
                else if(xmlStrEqual(node->name,(xmlChar*)"AccessMode")){
                    /*The content of the node is saved in accessm*/
                    xmlChar*accessm = xmlNodeGetContent(node);
                    /*With these conditionals accessm will acquire te value of the corresponding ep::type*/
                    if(xmlStrEqual(accessm,(xmlChar*)"RO")){
                        accessmode=EP_RO;
                    }

                    else if(xmlStrEqual(accessm,(xmlChar*)"RW")){
                        accessmode = EP_RW;
                    }

                    else if(xmlStrEqual(accessm,(xmlChar*)"WO")){
                        accessmode = EP_WO;
                    }

                    dptr = (ep::DataNode*)nodeset.get(i);
                }

                /*If the name of the node is equal to sign then...*/
                else if (xmlStrEqual(node->name,(xmlChar*)"Sign")){
                    /*The content of the node is saved in the variable sign*/
                    sign = (char*)xmlNodeGetContent(node);
                    ssign = sign;
                }

                /*If the name of the node is equal to pvalue then..*/
                else if(xmlStrEqual(node->name,(xmlChar*)"pValue")){
                    /*The content of the node is saved in the variable pvalue*/
                    pvalue = (char*)xmlNodeGetContent(node);
                    std::string spvalue = pvalue;
                    /*This funtion is called to parse the content of a pValue element*/
                    char*parentname = (char*)parent->name;
                    std::string sparentname =parentname;
                    parsePvalue(parent,nodeset.get(i),pvalue,parentname);
                }

                /*If the name of the node is equal to Min/Max/Inc then...*/
                else if ((xmlStrEqual(node->name,(xmlChar*)"Min"))||(xmlStrEqual(node->name,(xmlChar*)"Max"))||
                (xmlStrEqual(node->name,(xmlChar*)"Inc"))){
                    
                    ep::DataNode* datnode = (ep::DataNode*)nodeset.get(i);
                    std::string a(p);
                    void*ptr_inp;
                        switch (datnode->datatype())  {
                        case EP_8C:{
                            char*ptr8c = new char(*p);
                            ptr_inp = ptr8c;
                            newdata.push_back(*ptr8c);
                        }
                        break;
                        case EP_8S:{
                            char*ptr8s = new char(*p);
                            ptr_inp = ptr8s;
                            newdata.push_back(*ptr8s);}
                        break;
                        case EP_8U:{
                            char*ptr8u = new char(*p);
                            ptr_inp = ptr8u;
                            newdata.push_back(*ptr8u);}
                        break;
                        case EP_16U:{
                            unsigned short shr = (unsigned short)strtoull(p,NULL,10);
                            unsigned short*ptr16u = new unsigned short(shr);
                            ptr_inp = ptr16u;
                            newdata.push_back(*ptr16u);}
                        break;
                        case EP_16S:{
                            short shr = (short)strtol(p,NULL,10);
                            short*ptr16s = new short (shr);
                            ptr_inp = ptr16s;
                            newdata.push_back(*ptr16s);
                        }
                        break;
                        case EP_32S:{
                            int intt = (int)strtol(p,NULL,10);
                            int*ptr32s = new int(intt);
                            ptr_inp = ptr32s;
                            newdata.push_back(*ptr32s);
                        }

                        break;
                        case EP_32U:{
                            unsigned int usint = (unsigned int)strtoull(p,NULL,10);
                            unsigned int*ptr32u = new unsigned int(usint);
                            ptr_inp = ptr32u;
                            newdata.push_back(*ptr32u);

                        }
                        break;
                        case EP_64U:{
                            unsigned long uslong = (unsigned long)strtoull(p,NULL,10);
                            unsigned long*ptr64u = new unsigned long(uslong);
                            ptr_inp = ptr64u;
                            newdata.push_back(*ptr64u);
                        }
                        break;
                        case EP_64S:{
                            long oolong = strtol(p,NULL,10);
                            long*ptr64s = new long(oolong);
                            ptr_inp = ptr64s;
                            newdata.push_back(*ptr64s);
                        }
                        break;
                        case EP_32F:{
                            float f = std::stof(a);
                            float*ptr32f = new float(f);
                            ptr_inp = ptr32f;
                            newdata.push_back(*ptr32f);
                        }
                        break;
                        case EP_64F:{
                            double d = std::stod(a);
                            double*ptr64f = new double(d);
                            ptr_inp = ptr64f;
                            newdata.push_back(*ptr64f);
                        }

                        break;
                        case EP_CHAR256:

                        break;
                        default:

                        break;
                            }
                    
                    std::string sname = name;
                    ep::DataNode*ddptr=new ep::DataNode(sname,datnode->datatype(),ptr_inp);


                    if(xmlStrEqual(node->name,(xmlChar*)"Min")){
                        nodeset.get(i)->addReference(EP_HASMIN,ddptr);
                        nodeset.add(ddptr);
                    }
                    else if(xmlStrEqual(node->name,(xmlChar*)"Max")){
                        nodeset.get(i)->addReference(EP_HASMAX,ddptr);
                        nodeset.add(ddptr);
                    }
                    else if (xmlStrEqual(node->name,(xmlChar*)"Inc")){
                        nodeset.get(i)->addReference(EP_HASINCREMENT,ddptr);
                        nodeset.add(ddptr);
                    }
                }

                else if ((xmlStrEqual(node->name,(xmlChar*)"pMin"))||(xmlStrEqual(node->name,(xmlChar*)"pMax"))){
                    std::string a(p);
                    std::string sname = name;
                    ep::StringNode *sptr = new ep::StringNode(sname,a);
                    if(xmlStrEqual(node->name,(xmlChar*)"pMin")){
                        nodeset.get(i)->addReference(EP_HASMIN,sptr);
                        nodeset.add(sptr);
                    }
                    else if(xmlStrEqual(node->name,(xmlChar*)"pMax")){
                        nodeset.get(i)->addReference(EP_HASMAX,sptr);
                        nodeset.add(sptr);
                    }


                }
            }
            
    if (hasvalue){
        for(long unsigned int i = 0;i !=dnode->references().size();i++){
        ep::Node2*ptr = (ep::Node2*) dnode->references()[i]->address();
        if(ptr->nodetype()==EP_DATANODE){
            ep::DataNode*data = (ep::DataNode*)ptr;
            if(data->datatype() == 0){
                data->setDatatype(dnode->datatype());
            }
        }
        }}
            
        }
        node = node->next;
    }
    /*If the variable sign and size are not null then both variables are checked and a datatype is assigned*/
    if(sign!=NULL && size !=NULL){
        std::string num = size;
        if(ssign==std::string("Unsigned")){
            if(ssign==std::string("Unsigned")){
                if(num =="1"&&parentname == std::string("Boolean")){
                    datatype = EP_BOOL;
                }
                else if(num =="1"){
                    datatype = EP_8U;
                }
                else if(num =="2"){
                    datatype = EP_16U;
                }
                else if((num =="4")&&(parentname != std::string("Float"))){
                    datatype = EP_32U;
                }
                else if((num =="4")&&(parentname == std::string("Float"))){
                    datatype = EP_32F;
                }
                else if(num=="8"){
                    datatype = EP_64U;
                }}
            else{
                if(num =="1"&&parentname == std::string("Boolean")){
                    datatype = EP_BOOL;
                }
                else if(num =="1"){
                    datatype =EP_8S;
                }
                else if(num =="2"){
                    datatype= EP_16S;
                }
                else if((num =="4")&&(parentname == std::string("Float"))){
                    datatype = EP_32F;
                }
                else if((num =="4")&&(parentname != std::string("Float"))){
                    datatype = EP_32S;
                }
                else if ((num=="8")&&(parentname == std::string("Float"))){
                    datatype = EP_64F;
                }
                else if ((num=="8")&&(parentname != std::string("Float"))){
                    datatype = EP_64S;
                }
        }
    dptr->setAccessMode(accessmode);
    dptr->setDatatype(datatype);
    }
    }
}


void GenicamXMLParser::parsePvalue(xmlNode*node,ep::Node2* nnode,std::string nodename,std::string parentname){
    /*Inicialization of some variables*/
    xmlNode *cur_node = NULL;
    char* size = NULL;
    char* sign;
    int* datavalue = new int(0);
    ep::type datatype;
    std::string ssign;
    ep::type accessmode = 0;

            /*Loop for to find the node that has the same name as the pvalue(nodename)*/
            for(cur_node = node; cur_node; cur_node = cur_node->next){
                
                /*If the node name is not text and the nnode(node in nodeset) is equal to a data node then.. */
                if ((cur_node->type == XML_ELEMENT_NODE)&&
                !(xmlStrEqual(cur_node->name,(xmlChar*)"text"))
                &&(nnode->nodetype()==EP_DATANODE)){
                    /*Get the propertie of the node(Name)*/
                    xmlAttr* prop = cur_node->properties;
                    /*Get the value of the propertie(in this case the value of Name)*/
                    char* value = (char*)xmlGetProp(cur_node,prop->name);
                    svalue = value;

                    /*If svalue(Content of the propertie,Name) is equal to nodename then...*/
                    if (svalue == nodename){
                        xmlNode*nodec = cur_node->children;

                        /*As long as the child node is not null*/
                        while(nodec != NULL){
                            /*Get the content of the actual node*/
                            char* p = (char*)xmlNodeGetContent(nodec);
                            std::string s = p;
                            /*Get the name of the actual node*/
                            char* name = (char*)nodec->name;
                            std::string sname = name;
                            ep::DataNode* dptrrank = (ep::DataNode*)nnode;

                            if(xmlStrEqual(nodec->name,(xmlChar*)"pSelected")){
                                dptrrank->setRank(2);
                            }
                            else if(dptrrank->rank()==0){
                                dptrrank->setRank(1);
                            }

                            /*If the name of the actual node is equal to Description then...*/
                            if (xmlStrEqual((xmlChar*)nodec->name,(xmlChar*)"Length")){
                                /*The content of the node(variable s) is saved in the varible class tooltip*/
                                size =(char*)xmlNodeGetContent(nodec);
                            }

                            // /*If the name of the actual node is equal to Description then...*/
                            // if (xmlStrEqual((xmlChar*)nodec->name,(xmlChar*)"Address")){
                            //     /*The content of the node(variable s) is saved in the varible class tooltip*/
                            //     char*dataname = strcpy(new char[nnode->name().length()+1],nnode->name().c_str());
                            //     //if(nnode->references().size() == 0){
                            //         std::cout<<"????????????????????"<<std::endl;
                            //         this->parcam->getDeviceSettingValue(dataname,datavalue);
                            //         std::cout<<dataname<<" = "<<*datavalue<<std::endl;
                            //         //dptrrank->setValue(datavalue);
                            //         delete(dataname);
                            //         //}
                            // }

                            /*If the name of the actual node is equal to Accessmode then...*/
                            else if (xmlStrEqual((xmlChar*)nodec->name,(xmlChar*)"AccessMode")){
                                /*The content of the node is saved in accessm*/
                                xmlChar*accessm = xmlNodeGetContent(nodec);
                                /*With these conditionals accessm will acquire te value of the corresponding ep::type*/
                                if(xmlStrEqual(accessm,(xmlChar*)"RO")){
                                    accessmode=EP_RO;
                                }
                                else if(xmlStrEqual(accessm,(xmlChar*)"RW")){
                                    accessmode = EP_RW;
                                }
                                else if(xmlStrEqual(accessm,(xmlChar*)"WO")){
                                    accessmode = EP_WO;
                                }
                            }
                            /*If the name of the node is equal to sign then...*/
                            else if (xmlStrEqual(nodec->name,(xmlChar*)"Sign")){
                                /*The content of the node is saved in the variable sign*/
                                sign = (char*)xmlNodeGetContent(nodec);
                                ssign = sign;
                            }
                            /*If the name of the node is equal to pvalue then..*/
                            else if(xmlStrEqual(nodec->name,(xmlChar*)"pValue")){
                                /*The content of the node is saved in the variable pvalue*/
                                pvalue = (char*)xmlNodeGetContent(nodec);
                                std::string spvalue = pvalue;
                                /*This funtion is called to parse the content of a pValue element*/
                                parsePvalue(cur_node,nnode,spvalue,parentname);

                            }
                                nodec = nodec ->next;
                        
                        }
                        if(size !=NULL){
                            std::string num = size;
                            if(ssign==std::string("Unsigned")){
                                if(num =="1"&&parentname == "Boolean"){
                                    datatype = EP_BOOL;
                                }
                                else if(num =="1"){
                                    datatype = EP_8U;
                                }
                                else if(num =="2"){
                                    datatype = EP_16U;
                                }
                                else if((num =="4")&&(parentname != "Float")){
                                    datatype = EP_32U;
                                }
                                else if((num =="4")&&(parentname == "Float")){
                                    datatype = EP_32F;
                                }
                                else if(num=="8"){
                                    datatype = EP_64U;
                                }}
                            else{
                                if(num =="1"&&parentname == "Boolean"){
                                    datatype = EP_BOOL;
                                }
                                else if(num =="1"){
                                    datatype =EP_8S;
                                }
                                else if(num =="2"){
                                    datatype= EP_16S;
                                }
                                else if((num =="4")&&(parentname == "Float")){
                                    datatype = EP_32F;
                                }
                                else if((num =="4")&&(parentname != "Float")){
                                    datatype = EP_32S;
                                }
                                else if ((num=="8")&&(parentname == "Float")){
                                    datatype = EP_64F;
                                }
                                else if ((num=="8")&&(parentname != "Float")){
                                    datatype = EP_64S;
                                }
                            }
                            ep::DataNode* dptr = (ep::DataNode*)nnode;
                            dptr->setAccessMode(accessmode);
                            dptr->setDatatype(datatype);
                            break;
                        }}
                    
                    }}
}

void GenicamXMLParser::parseComNode(xmlNode*node){
    /*Save the actual node as a variable called parent*/
    xmlNode* parent = node;
    /*Get the propertie of the actual Node (In this case the propertie Name)*/
    xmlAttr* prop = node->properties;
    /*Get the value of the propertie*/
    char* value = (char*)xmlGetProp(node,prop->name);
    /*Save the variable node as the child of the actual node*/
    node = node->children;

    /*As long as the child node is not null*/
    while(node != NULL){

        /*We look in all the elements of nodeset*/
        for(int i = 0; i!= nodeset.length();i++){
            ep::Node2* attrref = (ep::Node2*) nodeset.get(i);
            std::string nodename = attrref->name();
            svalue = value;

            /*If svalue(Content of the propertie,Name) is equal to nodename then...*/
            if (svalue == nodename){
                char* p = (char*)xmlNodeGetContent(node);
                char* name = (char*)node->name;
                std::string s = p;
                std::string sname = name;

                if (xmlStrEqual(node->name,(xmlChar*)"Description")){
                    nodeset.get(i)->setTooltip(s);
                }
                else if (xmlStrEqual(node->name,(xmlChar*)"CommandValue")){
                    ep::CommandNode* cptr = new ep::CommandNode(sname,&s);
                    nodeset.get(i)->addReference(EP_HASCOMMANDPARAMETER,cptr);
                }
                else if(xmlStrEqual(node->name,(xmlChar*)"pValue")){
                    pvalue = (char*)xmlNodeGetContent(node);
                    std::string spvalue = pvalue;
                    char*parentname = (char*)parent->name;
                    std::string sparentname =parentname;
                    parsePvalue(parent,nodeset.get(i),pvalue,parentname);
                }

            }
            
        }
        node = node->next;
    }
}
