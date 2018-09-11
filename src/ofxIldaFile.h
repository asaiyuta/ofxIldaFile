#pragma once
#include "ofMain.h"

//ref : ILDA Image Data Transfer Format Specification revision 011

#define OFX_ILDA_CONVERT
#ifdef OFX_ILDA_CONVERT
#include "ofxIldaFrame.h"
#endif

namespace ofx{
    namespace IldaFile{
        namespace util{
            template<typename T>
            T reverse_16b(T value){
                value = static_cast<T> (( value & 0x00FF ) << 8 | ( value & 0xFF00 ) >> 8 );
                return value;
            }
            
            template<typename T>
            T reverse_32b(T value){
                value = static_cast<T> (( value & 0x00FF00FF ) << 8 | ( value & 0xFF00FF00 ) >> 8 );
                value = static_cast<T> (( value & 0x0000FFFF ) << 16 | ( value & 0xFFFF0000 ) >> 16 );
                return value;
            }
        };
        
        enum FORMAT{
            Coordinates3D = 0,
            Coordinates2D = 1,
            ColorPalette = 2,
            Coordinates3DwTrueColor = 4,
            Coordinates2DwTrueColor = 5,
        };
        
        struct ilda_section_base{
            FORMAT format;
            std::string name;
            std::string company_name;
            uint16_t number_of_records;
            uint16_t frame_number;
            uint16_t total_frames;
            uint8_t projector_number;
            uint8_t none;
        };
        
        template<FORMAT format_type>
        struct ilda_section : ilda_section_base{};
        
        template<> struct ilda_section<FORMAT::Coordinates3D> : ilda_section_base{ std::vector<std::tuple<ofVec3f, uint8_t, uint8_t>> data; };
        template<> struct ilda_section<FORMAT::Coordinates2D> : ilda_section_base{ std::vector<std::tuple<ofVec2f, uint8_t, uint8_t>> data; };
        template<> struct ilda_section<FORMAT::ColorPalette> : ilda_section_base{std::array<ofColor, 255> data; };
        template<> struct ilda_section<FORMAT::Coordinates3DwTrueColor> : ilda_section_base{ std::vector<std::tuple<ofVec3f, uint8_t, ofColor>> data; };
        template<> struct ilda_section<FORMAT::Coordinates2DwTrueColor> : ilda_section_base{ std::vector<std::tuple<ofVec2f, uint8_t, ofColor>> data; };
        
        namespace load_functions{
            namespace commons{
                const bool read_ilda(std::ifstream& ifs){
                    char head[5];
                    head[4] = NULL;
                    ifs.read((char*)&head,4);
                    return (strncmp(head,"ILDA",4) == 0);
                }
                
                void read_header(std::shared_ptr<ilda_section_base>& section_base, std::ifstream& ifs){
                    char name_buf[9];
                    name_buf[8] = NULL;
                    ifs.read((char*)&name_buf, 8);
                    section_base -> name = std::string(name_buf);
                    ifs.read((char*)&name_buf, 8);
                    section_base -> company_name = std::string(name_buf);
                    uint16_t bit16_buf;
                    ifs.read((char*)&bit16_buf, 2);
                    section_base -> number_of_records = util::reverse_16b(bit16_buf);
                    ifs.read((char*)&bit16_buf, 2);
                    section_base -> frame_number = util::reverse_16b(bit16_buf);
                    ifs.read((char*)&bit16_buf, 2);
                    section_base -> total_frames = util::reverse_16b(bit16_buf);
                    ifs.read((char*)&section_base -> projector_number, 1);
                    ifs.read((char*)&section_base -> none, 1);
                }
            };
            
            template<FORMAT format> void type_load(ilda_section<format>& section, std::ifstream& ifs){}
            template<> void type_load(ilda_section<FORMAT::Coordinates3D>& section, std::ifstream& ifs){
                //TODO:
            }
            template<> void type_load(ilda_section<FORMAT::Coordinates2D>& section, std::ifstream& ifs){
                //TODO:
            }
            template<> void type_load(ilda_section<FORMAT::ColorPalette>& section, std::ifstream& ifs){
                //TODO:
            }
            
            template<> void type_load(ilda_section<FORMAT::Coordinates3DwTrueColor>& section, std::ifstream& ifs){
                int16_t pos_buf[3];
                uint8_t col_buf[3];
                
                for(std::size_t i = 0 ; i <  section.number_of_records ; ++i){
                    section.data.emplace_back();
                    auto& d = section.data.back();
                    auto& pos = std::get<0>(d);
                    uint8_t& status = std::get<1>(d);
                    auto& color = std::get<2>(d);
          
                    ifs.read((char*)&pos_buf, 6);
                    ifs.read((char*)&status, 1);
                    ifs.read((char*)&col_buf, 3);
                    
                    pos.x = util::reverse_16b(pos_buf[0]);
                    pos.y = util::reverse_16b(pos_buf[1]);
                    pos.z = util::reverse_16b(pos_buf[2]);
                    
                    color.set(col_buf[0], col_buf[1], col_buf[2]);
                }
            }
            
            template<> void type_load(ilda_section<FORMAT::Coordinates2DwTrueColor>& section, std::ifstream& ifs){
                int16_t pos_buf[2];
                uint8_t col_buf[2];
                
                for(std::size_t i = 0 ; i <  section.number_of_records ; ++i){
                    section.data.emplace_back();
                    auto& d = section.data.back();
                    auto& pos = std::get<0>(d);
                    uint8_t& status = std::get<1>(d);
                    auto& color = std::get<2>(d);
                    
                    ifs.read((char*)&pos_buf, 4);
                    ifs.read((char*)&status, 1);
                    ifs.read((char*)&col_buf, 3);
                    
                    pos.x = util::reverse_16b(pos_buf[0]);
                    pos.y = util::reverse_16b(pos_buf[1]);
                    
                    color.set(col_buf[0], col_buf[1], col_buf[2]);
                }
            }
            
            const bool load_section(std::shared_ptr<ilda_section_base>& section_base, std::ifstream& ifs){
                commons::read_ilda(ifs);
                ifs.seekg(3 * sizeof(char), std::ios::cur);
                uint8_t type;
                ifs.read((char*)&type, 1);
                
                switch ((FORMAT)type){
                    case FORMAT::Coordinates3D :
                        section_base.reset(new ilda_section<FORMAT::Coordinates3D>());
                        break;
                    
                    case FORMAT::Coordinates2D :
                        section_base.reset(new ilda_section<FORMAT::Coordinates2D>());
                        break;
                    
                    case FORMAT::ColorPalette :
                        section_base.reset(new ilda_section<FORMAT::ColorPalette>());
                        break;
                    
                    case FORMAT::Coordinates3DwTrueColor :
                        section_base.reset(new ilda_section<FORMAT::Coordinates3DwTrueColor>());
                        break;
                    
                    case FORMAT::Coordinates2DwTrueColor :
                        section_base.reset(new ilda_section<FORMAT::Coordinates2DwTrueColor>());
                        break;
                    default:
                        break;
                }
                
                section_base -> format = (FORMAT)type;
                commons::read_header(section_base, ifs);
                
                switch (section_base -> format){
                    case FORMAT::Coordinates3D :
                        type_load(*(ilda_section<FORMAT::Coordinates3D>*)section_base.get(), ifs);
                        break;
                    
                    case FORMAT::Coordinates2D :
                        type_load(*(ilda_section<FORMAT::Coordinates2D>*)section_base.get(), ifs);
                        break;
                    
                    case FORMAT::ColorPalette :
                        type_load(*(ilda_section<FORMAT::ColorPalette>*)section_base.get(), ifs);
                        break;
                    
                    case FORMAT::Coordinates3DwTrueColor :
                        type_load(*(ilda_section<FORMAT::Coordinates3DwTrueColor>*)section_base.get(), ifs);
                        break;
                    
                    case FORMAT::Coordinates2DwTrueColor :
                        type_load(*(ilda_section<FORMAT::Coordinates2DwTrueColor>*)section_base.get(), ifs);
                        break;
                    default:
                        break;
                }
            }
        };
        
        namespace write_functions{
            namespace commons{
                void write_header(std::shared_ptr<ilda_section_base>& section_base, std::ofstream& ofs){
                    char head[5] = "ILDA";
                    ofs.write((char*)head,4);
                    uint8_t type_buf[4] = {0,0,0, uint8_t(section_base -> format)};
                    ofs.write((char*)type_buf, 4);
                    ofs.write((char*)section_base -> name.c_str(), 8);
                    ofs.write((char*)section_base -> company_name.c_str(), 8);
                    uint16_t bit16_buf = util::reverse_16b(section_base -> number_of_records);
                    ofs.write((char*)&bit16_buf, 2);
                    bit16_buf = util::reverse_16b(section_base -> frame_number);
                    ofs.write((char*)&bit16_buf, 2);
                    bit16_buf = util::reverse_16b(section_base -> total_frames);
                    ofs.write((char*)&bit16_buf, 2);
                    ofs.write((char*)&section_base -> projector_number, 1);
                    ofs.write((char*)&section_base -> none, 1);
                }
            };
            
            template<FORMAT format> void type_write(ilda_section<format>& section, std::ofstream& ofs){}
            template<> void type_write(ilda_section<FORMAT::Coordinates3D>& section, std::ofstream& ofs){
                //TODO:
            }
            template<> void type_write(ilda_section<FORMAT::Coordinates2D>& section, std::ofstream& ofs){
                //TODO:
            }
            template<> void type_write(ilda_section<FORMAT::ColorPalette>& section, std::ofstream& ofs){
                //TODO:
            }
            
            template<> void type_write(ilda_section<FORMAT::Coordinates3DwTrueColor>& section, std::ofstream& ofs){
                int16_t pos_buf[3];
                uint8_t col_buf[3];

                for(std::size_t i = 0 ; i <  section.number_of_records ; ++i){
                    auto& d = section.data[i];
                    auto& pos = std::get<0>(d);
                    uint8_t& status = std::get<1>(d);
                    auto& color = std::get<2>(d);

                    pos_buf[0] = pos.x;
                    pos_buf[0] = util::reverse_16b(pos_buf[0]);
                    pos_buf[1] = pos.y;
                    pos_buf[1] = util::reverse_16b(pos_buf[1]);
                    pos_buf[2] = pos.z;
                    pos_buf[2] = util::reverse_16b(pos_buf[2]);

                    col_buf[0] = color.r;
                    col_buf[1] = color.g;
                    col_buf[2] = color.b;
                    
                    ofs.write((char*)&pos_buf, 6);
                    ofs.write((char*)&status, 1);
                    ofs.write((char*)&col_buf, 3);
                }
            }
            
            template<> void type_write(ilda_section<FORMAT::Coordinates2DwTrueColor>& section, std::ofstream& ofs){
                int16_t pos_buf[2];
                uint8_t col_buf[3];
                
                for(std::size_t i = 0 ; i <  section.number_of_records ; ++i){
                    auto& d = section.data[i];
                    auto& pos = std::get<0>(d);
                    uint8_t& status = std::get<1>(d);
                    auto& color = std::get<2>(d);
                    
                    pos_buf[0] = pos.x;
                    pos_buf[0] = util::reverse_16b(pos_buf[0]);
                    pos_buf[1] = pos.y;
                    pos_buf[1] = util::reverse_16b(pos_buf[1]);
                    
                    col_buf[0] = color.r;
                    col_buf[1] = color.g;
                    col_buf[2] = color.b;
                    
                    ofs.write((char*)&pos_buf, 4);
                    ofs.write((char*)&status, 1);
                    ofs.write((char*)&col_buf, 3);
                }
            }
            
            void write_sections(std::shared_ptr<ilda_section_base>& section_base, std::ofstream& ofs){
                commons::write_header(section_base, ofs);
                switch (section_base -> format){
                    case FORMAT::Coordinates3D :
                        type_write(*(ilda_section<FORMAT::Coordinates3D>*)section_base.get(), ofs);
                        break;
                    
                    case FORMAT::Coordinates2D :
                        type_write(*(ilda_section<FORMAT::Coordinates2D>*)section_base.get(), ofs);
                        break;
                    
                    case FORMAT::ColorPalette :
                        type_write(*(ilda_section<FORMAT::ColorPalette>*)section_base.get(), ofs);
                        break;
                    
                    case FORMAT::Coordinates3DwTrueColor :
                        type_write(*(ilda_section<FORMAT::Coordinates3DwTrueColor>*)section_base.get(), ofs);
                        break;
                    
                    case FORMAT::Coordinates2DwTrueColor :
                        type_write(*(ilda_section<FORMAT::Coordinates2DwTrueColor>*)section_base.get(), ofs);
                        break;
                    default:
                        break;
                }
            }
        };
        
        struct ilda_file{
            void load(std::string path){
                std::ifstream ifs(path, std::ios::binary);
                if(ifs){
                    ifs.seekg(0, std::ios_base::end);
                    ifstream::pos_type file_size = ifs.tellg();
                    ifs.clear();
                    ifs.seekg(0, std::ios_base::beg);
                    ofLogNotice("ofxIldaFile") << "succes open file file size : " << file_size;
                    std::vector<ifstream::pos_type> section_heads;
                    
                    uint8_t eof_buf;
                    while(ifs.tellg() >= 0){
                        ifstream::pos_type current = ifs.tellg();
                        if(load_functions::commons::read_ilda(ifs)){
                            section_heads.push_back(current);
                        }else{
                            ifs.seekg(-3, std::ios_base::cur);
                            ifs.read((char*)&eof_buf, 1);
                        }
                    }
                    ofLogNotice("ofxIldaFile") << "num sections " << section_heads.size();
                    ifs.clear();
                    for(auto& e : section_heads){
                        ilda_sections.push_back(std::shared_ptr<ilda_section_base>());
                        ifs.seekg(e, std::ios_base::beg);
                        load_functions::load_section(ilda_sections.back(), ifs);
                    }
                    ifs.close();
                }else{
                    ofLogError("ofxIldaFile", "filed open file");
                }
            }
            
            void load_thread_start(std::string path){
                path_buf = path;
                std::thread([&](){
                    load_mutex.lock();
                    load(path_buf);
                    load_mutex.unlock();
                }).detach();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            void load_thread_end(){
                load_mutex.lock();
                load_mutex.unlock();
            }
            
            void write(std::string path){
                std::ofstream ofs(path, std::ios::binary);
                if(ofs){
                    ofLogNotice("ofxIldaFile") << "succes open file";
                    ofLogNotice("ofxIldaFile") << "num sections " << ilda_sections.size();
                    for(auto& e : ilda_sections){
                        write_functions::write_sections(e, ofs);
                    }
                    ofs.close();
                    ofLogNotice("ofxIldaFile") << "finish save";
                }else{
                    ofLogError("ofxIldaFile", "filed open file");
                }
            }
            
            
            
            const std::string test_dev_draw(std::size_t index, float scale = ofGetHeight() / 2.0){
                ofPushStyle();
                scale /= 32767.0;
                index = index % ilda_sections.size();
                auto& section = ilda_sections[index];
                if(section -> format == FORMAT::Coordinates3DwTrueColor){
                    const auto& frame = *(ilda_section<FORMAT::Coordinates3DwTrueColor>*)section.get();
                    for(std::size_t i = 1; i < frame.data.size() ; ++i){
                        const auto& b = frame.data[i - 1];
                        const auto& e = frame.data[i];
                        if(!(std::get<1>(b) & (1 << 6))){
                            const auto& b_col = std::get<2>(b);
                            ofSetColor(b_col);
                            ofDrawCircle(std::get<0>(b) * scale, 1.5);
                            if(!(std::get<1>(e) & (1 << 6))){
                                const auto& e_col = std::get<2>(e);
                                ofSetColor((b_col.r + e_col.r) * 0.5, (b_col.g + e_col.g) * 0.5, (b_col.b + e_col.b) * 0.5);
                                ofDrawLine(std::get<0>(b) * scale, std::get<0>(e) * scale);
                            }
                        }
                    }
                    
                    if(frame.data.size()){
                        auto& rast = frame.data.back();
                        if(!(std::get<1>(rast) & (1 << 6))){
                            ofSetColor(std::get<2>(rast));
                            ofDrawCircle(std::get<0>(rast) * scale, 1.5);
                        }
                    }
                }
                ofPopStyle();
                std::stringstream ss("");
                ss
                << "index              : " << index << "\n"
                << "frame name         : " << section -> name << "\n"
                << "company name       : " << section -> company_name << "\n"
                << "number_of_records  : " << section -> number_of_records << "\n"
                << "frame number       : " << section -> frame_number << "\n"
                << "total frames       : " << section -> total_frames << std::endl;
                return ss.str();
            }
            
            std::vector<std::shared_ptr<ilda_section_base>> ilda_sections;
        private:
            std::string path_buf;
            std::mutex load_mutex;
        };
        
#ifdef OFX_ILDA_CONVERT
        struct section_converter{
            void set_frame(uint16_t frame_number, const std::vector<ofxIlda::Point>& points){
                if(frame_buffer.count(frame_number)){
                    frame_buffer[frame_number].resize(points.size());
                    std::copy(points.begin(), points.end(), frame_buffer[frame_number].begin());
                }else{
                    frame_buffer[frame_number] = points;
                }
            }
            
            void to_file(ilda_file& file, std::string frame_name = "hogehoge", std::string company_name = "ofxIldaF"){
                auto& sections = file.ilda_sections;
                uint16_t total_frame = get_max_frame();
                uint16_t section_total_frame = total_frame + 1;
                std::shared_ptr<ilda_section_base> pre_frame_ptr(new ilda_section<FORMAT::Coordinates2DwTrueColor>());
                {
                    auto& pre_frame = *(ilda_section<FORMAT::Coordinates2DwTrueColor>*)pre_frame_ptr.get();
                    pre_frame.format = FORMAT::Coordinates2D;
                    pre_frame.name = frame_name;
                    pre_frame.company_name = company_name;
                    pre_frame.number_of_records = 1;
                    pre_frame.frame_number = 0;
                    pre_frame.total_frames = section_total_frame;
                    pre_frame.projector_number = 0;
                    pre_frame.none = 0;
                    pre_frame.data.resize(1);
                    std::get<0>(pre_frame.data[0]).set(0, 0);
                    std::get<1>(pre_frame.data[0]) = 0b01000000;
                    std::get<2>(pre_frame.data[0]).set(0,0,0,255);
                }
                for(uint16_t f = 0 ; f < total_frame ; ++f){
                    std::shared_ptr<ilda_section_base> current_frame_ptr(new ilda_section<FORMAT::Coordinates2DwTrueColor>(*(ilda_section<FORMAT::Coordinates2DwTrueColor>*)pre_frame_ptr.get()));
                    if(frame_buffer.count(f)){
                        const auto& ilda_frame = frame_buffer.at(f);
                        auto& current_frame = *(ilda_section<FORMAT::Coordinates2DwTrueColor>*)current_frame_ptr.get();
                        current_frame.number_of_records = ilda_frame.size();
                        current_frame.frame_number = f;
                        current_frame.projector_number = 0;
                        current_frame.none = 0;
                        current_frame.data.resize(ilda_frame.size());
                        for(uint16_t i = 0 ; i < current_frame.number_of_records ; ++i){
                            auto& section_frame_data = current_frame.data[i];
                            auto& ilda_point = ilda_frame[i];
                            std::get<0>(section_frame_data).set(ilda_point.x, ilda_point.y);
                            std::get<1>(section_frame_data) = 0b00000000;
                            float r,g,b;
                            r = 1.0 * ilda_point.r / kIldaMaxIntensity * 255.0;
                            g = 1.0 * ilda_point.g / kIldaMaxIntensity * 255.0;
                            b = 1.0 * ilda_point.b / kIldaMaxIntensity * 255.0;
                            std::get<2>(section_frame_data).set(r,g,b,255);
                        }
                    }else{
                        auto& current_frame = *(ilda_section<FORMAT::Coordinates2DwTrueColor>*)current_frame_ptr.get();
                        current_frame.frame_number = f;
                    }
                    pre_frame_ptr = current_frame_ptr;
                    sections.push_back(pre_frame_ptr);
                }
                {
                    std::shared_ptr<ilda_section_base> current_frame_ptr(new ilda_section<FORMAT::Coordinates2DwTrueColor>(*(ilda_section<FORMAT::Coordinates2DwTrueColor>*)pre_frame_ptr.get()));
                    auto& current_frame = *(ilda_section<FORMAT::Coordinates2DwTrueColor>*)current_frame_ptr.get();
                    current_frame.number_of_records = 0;
                    current_frame.frame_number = total_frame;
                    current_frame.data.resize(0);
  
                }
            }
            
        private:
            uint16_t get_max_frame() const{
                uint16_t _m = 0;
                for(auto& e : frame_buffer){
                    if( e.first > _m) _m = e.first;
                }
                return _m;
            }
            
            std::map<uint16_t ,std::vector<ofxIlda::Point>> frame_buffer;
        };
        
#endif
        
    };
};
