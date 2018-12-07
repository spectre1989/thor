using System;
using System.IO;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;

namespace BinToJson
{
    [DataContract]
    struct Vec3f
    {
        [DataMember]
        public float x;

        [DataMember]
        public float y;

        [DataMember]
        public float z;
    }

    [DataContract]
    struct Def
    {
        [DataMember(Order = 0)]
        public string name;

        [DataMember(Order = 1)]
        public string type;

        [DataMember(Order = 2)]
        public string obj;

        [DataMember(Order = 3)]
        public Group[] groups;
    }

    [DataContract]
    struct Group
    {
        [DataMember(Order = 0)]
        public string name;

        [DataMember(Order = 1)]
        public Vec3f position;

        [DataMember(Order = 2)]
        public Vec3f rotation;
    }

    [DataContract]
    struct Geobin
    {
        [DataMember(Order = 0)]
        public Def[] defs;

        [DataMember(Order = 1)]
        public Group[] refs;
    }

    class Program
    {
        static void SkipString(FileStream file)
        {
            byte[] buffer = new byte[2];
            file.Read(buffer, 0, 2);
            int byte_count = BitConverter.ToUInt16(buffer, 0);

            int bytes_misaligned = (byte_count + 2) & 3; // & 3 is equivalent to % 4
            if (bytes_misaligned > 0)
            {
                byte_count += (4 - bytes_misaligned);
            }

            file.Position += byte_count;
        }

        static string ReadString(FileStream file)
        {
            byte[] buffer = new byte[2];
            file.Read(buffer, 0, 2);
            int length = BitConverter.ToUInt16(buffer, 0);
            byte[] string_bytes = new byte[length];
            file.Read(string_bytes, 0, length);

            int bytes_misaligned = (length + 2) & 3; // & 3 is equivalent to % 4
            if (bytes_misaligned > 0)
            {
                file.Position += (4 - bytes_misaligned);
            }

            return System.Text.ASCIIEncoding.ASCII.GetString(string_bytes);
        }

        static Vec3f ReadVec3f(FileStream file)
        {
            Vec3f v = new Vec3f();

            byte[] buffer = new byte[4];

            file.Read(buffer, 0, 4);
            v.x = BitConverter.ToSingle(buffer, 0);
            file.Read(buffer, 0, 4);
            v.y = BitConverter.ToSingle(buffer, 0);
            file.Read(buffer, 0, 4);
            v.z = BitConverter.ToSingle(buffer, 0);

            return v;
        }

        static Group ReadGroup(FileStream file)
        {
            Group group = new Group();

            file.Position += 4; // size

            group.name = ReadString(file);
            group.position = ReadVec3f(file);
            group.rotation = ReadVec3f(file);

            file.Position += 4; // flags

            return group;
        }

        static void Main(string[] args)
        {
            Geobin geobin = new Geobin();

            FileStream in_file = File.OpenRead(args[0]);

            in_file.Position += 28;

            byte[] buffer = new byte[4];

            // skip files section
            in_file.Read(buffer, 0, 4);
            uint files_size = BitConverter.ToUInt32(buffer, 0);
            in_file.Position += files_size;

            // data size
            in_file.Position += 4;

            // version
            in_file.Position += 4;

            SkipString(in_file); // scene file
            SkipString(in_file); // loading screen

            in_file.Read(buffer, 0, 4);
            int def_count = BitConverter.ToInt32(buffer, 0);

            geobin.defs = new Def[def_count];

            for (int def_i = 0; def_i < def_count; ++def_i)
            {
                in_file.Read(buffer, 0, 4);
                uint size = BitConverter.ToUInt32(buffer, 0);
                long start = in_file.Position;

                geobin.defs[def_i].name = ReadString(in_file);

                in_file.Read(buffer, 0, 4);
                int group_count = BitConverter.ToInt32(buffer, 0);

                geobin.defs[def_i].groups = new Group[group_count];

                for (int group_i = 0; group_i < group_count; ++group_i)
                {
                    geobin.defs[def_i].groups[group_i] = ReadGroup(in_file);
                }

                for (int skip_i = 0; skip_i < 11; ++skip_i)
                {
                    in_file.Read(buffer, 0, 4);
                    int count = BitConverter.ToInt32(buffer, 0);
                    for (int i = 0; i < count; ++i)
                    {
                        in_file.Read(buffer, 0, 4);
                        in_file.Position += BitConverter.ToUInt32(buffer, 0);
                    }
                }

                geobin.defs[def_i].type = ReadString(in_file);
                in_file.Position += 4; // flags
                in_file.Position += 4; // alpha
                geobin.defs[def_i].obj = ReadString(in_file);

                in_file.Position = start + size;
            }

            in_file.Read(buffer, 0, 4);
            int ref_count = BitConverter.ToInt32(buffer, 0);

            geobin.refs = new Group[ref_count];

            for (int ref_i = 0; ref_i < ref_count; ++ref_i)
            {
                geobin.refs[ref_i] = ReadGroup(in_file);
            }

            in_file.Close();

            DataContractJsonSerializer serialiser = new DataContractJsonSerializer(typeof(Geobin));
            MemoryStream stream = new MemoryStream();
            var writer = JsonReaderWriterFactory.CreateJsonWriter(stream, System.Text.Encoding.UTF8, true, true);
            serialiser.WriteObject(writer, geobin);
            writer.Flush();
            stream.Position = 0;
            FileStream out_file = File.OpenWrite(args[0] + ".txt");
            out_file.Write(stream.GetBuffer(), 0, (int)stream.Length);
            out_file.Close();
        }
    }
}
