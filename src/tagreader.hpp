//   Copyright 2022 light - caozhanhao
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

#include "stream.hpp"
#include "utils.hpp"
#include "logger.hpp"

#include <cstdio>
#include <array>
#include <vector>
#include <locale>
#include <string>
#include <codecvt>

#ifndef LIGHT_TAGREADER_HPP
#define LIGHT_TAGREADER_HPP
namespace light::tagreader
{
  struct ID3v2Header
  {
    std::array<char, 3> header;
    char version;
    char revision;
    char flag;
    std::array<char, 4> size;
  };
  
  struct ID3v2Frame
  {
    std::array<char, 4> frame_id;
    std::array<char, 4> size;
    std::array<char, 2> flags;
  };
  const std::string audio_encryption = "AENC";
  const std::string attached_picture = "APIC";
  const std::string comments = "COMM";
  const std::string commercial_frame = "COMR";
  const std::string encryption_method_registration = "ENCR";
  const std::string equalization = "EQUA";
  const std::string event_timing_codes = "ETCO";
  const std::string general_encapsulated_object = "GEOB";
  const std::string group_identification_registration = "GRID";
  const std::string involved_people_list = "IPLS";
  const std::string linked_information = "LINK";
  const std::string music_CD_identifier = "MCDI";
  const std::string MPEG_location_lookup_table = "MLLT";
  const std::string ownership_frame = "OWNE";
  const std::string private_frame = "PRIV";
  const std::string play_counter = "PCNT";
  const std::string popularimeter = "POPM";
  const std::string position_synchronisation_frame = "POSS";
  const std::string recommended_buffer_size = "RBUF";
  const std::string relative_volume_adjustment = "RVAD";
  const std::string reverb = "RVRB";
  const std::string synchronized_lyric_or_text = "SYLT";
  const std::string synchronized_tempo_codes = "SYTC";
  const std::string album_or_movie_or_show_title = "TALB";
  const std::string beats_per_minute = "TBPM";
  const std::string BPM = "TBPM";
  const std::string composer = "TCOM";
  const std::string content_type = "TCON";
  const std::string copyright_message = "TCOP";
  const std::string playlist_delay = "TDLY";
  const std::string encoded_by = "TENC";
  const std::string lyricistor_or_text_writer = "TENC";
  const std::string file_type = "TFLT";
  const std::string time = "TIME";
  const std::string content_group_description = "TIT1";
  const std::string title_or_songname_or_content_description = "TIT2";
  const std::string subtitle_or_description_refinement = "TIT3";
  const std::string initial_key = "TKEY";
  const std::string language = "TLAN";
  const std::string length = "TLEN";
  const std::string media_type = "TMED";
  const std::string original_album_movie_show_title = "TOAL";
  const std::string original_filename = "TOFN";
  const std::string original_lyricist_or_text_writer = "TOLY";
  const std::string original_artist_or_performer = "TOPE";
  const std::string original_release_year = "TORY";
  const std::string file_owner_or_licensee = "TOWN";
  const std::string lead_performer_or_soloist = "TPE1";
  const std::string band_or_orchestra_or_accompaniment = "TPE2";
  const std::string conductor_or_performer_refinement = "TPE3";
  const std::string interpreted_remixed_or_otherwise_modified_by = "TPE4";
  const std::string part_of_a_set = "TPOS";
  const std::string publisher = "TPUB";
  const std::string track_number_or_position_in_set = "TRCK";
  const std::string recording_dates = "TRDA";
  const std::string internet_radio_station_name = "TRSN";
  const std::string internet_radio_station_owner = "TRSO";
  const std::string size = "TSIZ";
  const std::string international_standard_recording_code = "TSRC";
  const std::string ISRC = "TSRC";
  const std::string software_or_hardware_and_settings_used_for_encoding = "TSSE";
  const std::string year = "TYER";
  const std::string user_defined_text_information_frame = "TXXX";
  const std::string unique_file_identifier = "UFID";
  const std::string terms_of_use = "USER";
  const std::string unsychronized_lyric_text_transcription = "USLT";
  const std::string commercial_information = "WCOM";
  const std::string copyright_or_legal_information = "WCOP";
  const std::string official_audio_file_webpage = "WOAF";
  const std::string official_artist_or_performer_webpage = "WOAR";
  const std::string official_audio_source_webpage = "WOAS";
  const std::string official_internet_radio_station_homepage = "WORS";
  const std::string payment = "WPAY";
  const std::string publishers_official_webpage = "WPUB";
  const std::string user_defined_URL_link_frame = "WXXX";
  
  std::array<std::string, 75> ids{
      "AENC",
      "APIC",
      "COMM",
      "COMR",
      "ENCR",
      "EQUA",
      "ETCO",
      "GEOB",
      "GRID",
      "IPLS",
      "LINK",
      "MCDI",
      "MLLT",
      "OWNE",
      "PRIV",
      "PCNT",
      "POPM",
      "POSS",
      "RBUF",
      "RVAD",
      "RVRB",
      "SYLT",
      "SYTC",
      "TALB",
      "TBPM",
      "TBPM",
      "TCOM",
      "TCON",
      "TCOP",
      "TDLY",
      "TENC",
      "TENC",
      "TFLT",
      "TIME",
      "TIT1",
      "TIT2",
      "TIT3",
      "TKEY",
      "TLAN",
      "TLEN",
      "TMED",
      "TOAL",
      "TOFN",
      "TOLY",
      "TOPE",
      "TORY",
      "TOWN",
      "TPE1",
      "TPE2",
      "TPE3",
      "TPE4",
      "TPOS",
      "TPUB",
      "TRCK",
      "TRDA",
      "TRSN",
      "TRSO",
      "TSIZ",
      "TSRC",
      "TSRC",
      "TSSE",
      "TYER",
      "TXXX",
      "UFID",
      "USER",
      "USLT",
      "WCOM",
      "WCOP",
      "WOAF",
      "WOAR",
      "WOAS",
      "WORS",
      "WPAY",
      "WPUB",
      "WXXX",
  };
  
  class TagInfo
  {
  private:
    std::map<std::string, std::string> tags;
  public:
    TagInfo(std::shared_ptr<stream::InputStream> input)
    {
      ID3v2Header header;
      
      input->read(reinterpret_cast<unsigned char *>(&header), sizeof(ID3v2Header));
      if (std::string(header.header.data(), 3) != "ID3")
      {
        return;
      }
      
      std::size_t tag_size = (header.size[0] & 0x7f) * 0x200000
                             + (header.size[1] & 0x7f) * 0x400
                             + (header.size[2] & 0x7f) * 0x40
                             + (header.size[3] & 0x7f);
      
      ID3v2Frame frame;
      std::size_t bitscount = 0;
      std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> converter;
      while (bitscount < tag_size - sizeof(ID3v2Header))
      {
        bitscount += input->read(reinterpret_cast<unsigned char *>(&frame), sizeof(ID3v2Frame));
        
        std::size_t size = frame.size[0] * 0x10000000
                           + frame.size[1] * 0x10000 + frame.size[2] * 0x100 + frame.size[3];
        
        std::string id(frame.frame_id.data(), 4);
        if (size != 0 && id != attached_picture && std::find(ids.begin(), ids.end(), id) != ids.end())
        {
          std::size_t encoding = 0;
          bitscount += input->read(reinterpret_cast<unsigned char *>(&encoding), 1);
          // 0 -> GBK | 1 -> UTF-16 | 2 -> UTF-16BE | 3 -> UTF-8
          if (encoding == 0)//GBK
          {
            std::vector<char> show;
            show.resize((size - 1) / sizeof(char));
            bitscount += input->read(reinterpret_cast<unsigned char *>(show.data()), size - 1);
            tags[id] = std::string(show.data() + 1, (size - 1) / sizeof(char));
          }
          else if (encoding == 1)
          {
            std::vector<char16_t> show;
            show.resize((size - 1) / sizeof(char16_t));
            bitscount += input->read(reinterpret_cast<unsigned char *>(show.data()), size - 1);
            tags[id] = converter.to_bytes(std::u16string(show.data(), (size - 1) / sizeof(char16_t)));
          }
          else
          {
            input->ignore(size - 1);
          }
        }
        else
        {
          bitscount += size;
          input->ignore(size);
        }
      }
    }
    
    const std::string &operator[](const std::string &tag) const
    {
      auto it = tags.find(tag);
      if (it == tags.end())
      {
        throw logger::Error(LIGHT_ERROR_LOCATION, __func__, "Unexpected tag '" + tag + "'.");
      }
      return tags.at(tag);
    }
    
    std::string common_info() const
    {
      std::string ret;
      auto it = tags.find(title_or_songname_or_content_description);
      if (it != tags.end())
      {
        ret += it->second + " - ";
      }
      it = tags.find(lead_performer_or_soloist);
      if (it != tags.end())
      {
        ret += it->second;
      }
      return ret;
    }
    
    std::string all_info(const std::vector<std::string> &ignore = {attached_picture, comments}) const
    {
      std::string ret;
      for (auto it = tags.begin(); it != std::prev(tags.end()); ++it)
      {
        if (std::find(ignore.begin(), ignore.end(), it->first) != ignore.end()) continue;
        ret += it->second + " - ";
      }
      if (std::find(ignore.begin(), ignore.end(), tags.cbegin()->first) == ignore.end())
      {
        ret += tags.cbegin()->second;
      }
      return ret;
    }
  };
}
#endif