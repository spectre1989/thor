using System;
using System.Collections.Generic;

namespace Solver
{
    struct Multiplier
    {
        public int power;
        public string name;
    }

    struct Term
    {
        public int count;
        public Multiplier[] multipliers;
        public string dimension;

        Term(int count, List<Multiplier> multipliers, string dimension)
        {
            multipliers.Sort(delegate(Multiplier a, Multiplier b)
            {
                string[] possible_names = { "s", "i", "j", "k", "a", "b", "c" };
                return Array.IndexOf(possible_names, a.name).CompareTo(Array.IndexOf(possible_names, b.name));
            });

            while(true)
            {
                string old_dimension = dimension;
                
                dimension = dimension.Replace("xx", "");
                dimension = dimension.Replace("yy", "");
                dimension = dimension.Replace("zz", "");

                if (dimension == old_dimension)
                {
                    break;
                }
            }

            while(true)
            {
                bool pair_found = false;

                for (int i = 0; i < dimension.Length; ++i)
                {
                    char c = dimension[i];

                    string first_half = dimension.Substring(0, i + 1);
                    string second_half = dimension.Substring(i + 1);
                    if (second_half.Contains(c))
                    {
                        int index_in_second_half = second_half.IndexOf(c);

                        int num_shifts = index_in_second_half;
                        if (num_shifts % 2 == 1)
                        {
                            count *= -1;
                        }

                        dimension = first_half.Substring(0, i) + second_half.Remove(index_in_second_half, 1);

                        pair_found = true;
                        break;
                    }
                }

                if (!pair_found)
                {
                    break;
                }
            }

            if (dimension.Length == 3)
            {
                // always express trivectors as xyz
                if (dimension == "xzy" || dimension == "yxz" || dimension == "zyx")
                {
                    count *= -1;
                }

                dimension = "xyz";
            }
            else if (dimension.Length == 2)
            {
                if (dimension == "yz")
                {
                    count *= -1;
                    dimension = "zy";
                }
                else if (dimension == "zx")
                {
                    count *= -1;
                    dimension = "xz";
                }
                else if (dimension == "xy")
                {
                    count *= -1;
                    dimension = "yx";
                }
            }

            this.count = count;
            this.multipliers = multipliers.ToArray();
            this.dimension = dimension;
        }

        public static Term Parse(string s)
        {
            int sign = 1;
            List<Multiplier> multipliers = new List<Multiplier>();
            string dimension = "";

            int i = 0;
            while (i < s.Length)
            {
                char c = s[i];
                if (c == '-')
                {
                    sign = -1;
                }
                else
                {
                    bool is_dimension = (c == 'x' || c == 'y' || c == 'z');
                    if (is_dimension)
                    {
                        dimension = s.Substring(i, s.Length - i);
                        break;
                    }
                    else
                    {
                        Multiplier multiplier = new Multiplier();
                        multiplier.power = 1;
                        multiplier.name = c.ToString();

                        multipliers.Add(multiplier);
                    }
                }

                ++i;
            }

            return new Term(sign, multipliers, dimension);
        }

        public static Term[] Multiply(Term[] a, Term[] b)
        {
            List<Term> results = new List<Term>();

            // multiply
            foreach (Term term_in_a in a)
            {
                foreach (Term term_in_b in b)
                {
                    results.Add(Term.Multiply(term_in_a, term_in_b));
                }
            }

            // simplify
            while (true)
            {
                bool changed = false;
                for (int i = 0; i < (results.Count - 1); ++i)
                {
                    for (int j = i + 1; j < results.Count; ++j)
                    {
                        if (Term.CanCombine(results[i], results[j]))
                        {
                            Term combined = results[i];
                            combined.count += results[j].count;

                            results.RemoveAt(j);

                            if (combined.count != 0)
                            {
                                results[i] = combined;
                            }
                            else
                            {
                                results.RemoveAt(i);
                            }

                            changed = true;
                        }
                    }
                }

                if (!changed)
                {
                    break;
                }
            }

            return results.ToArray();
        }

        public static Term Multiply(Term a, Term b)
        {
            List<Multiplier> multipliers = new List<Multiplier>(a.multipliers);
            foreach (Multiplier mul_b in b.multipliers)
            {
                int index = multipliers.FindIndex(delegate(Multiplier item){ return item.name == mul_b.name;});
                if (index == -1)
                {
                    multipliers.Add(mul_b);
                }
                else
                {
                    Multiplier mul_a = multipliers[index];
                    mul_a.power += mul_b.power;
                    multipliers[index] = mul_a;
                }
            }

            return new Term(a.count * b.count, multipliers, a.dimension + b.dimension);
        }

        public static bool CanCombine(Term a, Term b)
        {
            if (a.dimension != b.dimension)
            {
                return false;
            }

            if (a.multipliers.Length != b.multipliers.Length)
            {
                return false;
            }

            for (int i = 0; i < a.multipliers.Length; ++i)
            {
                if (a.multipliers[i].name != b.multipliers[i].name)
                {
                    return false;
                }
                if (a.multipliers[i].power != b.multipliers[i].power)
                {
                    return false;
                }
            }

            return true;
        }

        public static string ToString(Term[] equation)
        {
            if (equation.Length == 0)
            {
                return "";
            }

            string s = "";

            if (equation[0].count < 0)
            {
                s += "-";
            } 
            s += ToString(equation[0]);

            for (int i = 1; i < equation.Length; ++i)
            {
                s += " ";

                if (equation[i].count < 0)
                {
                    s += "-";
                }
                else 
                { 
                    s += "+";
                }

                s += " " + ToString(equation[i]);
            }

            return s;
        }

        public static string ToString(Term term)
        {
            string s = "";

            if (term.count < -1 || term.count > 1)
            {
                s += Math.Abs(term.count).ToString();
            }

            foreach (Multiplier multiplier in term.multipliers)
            {
                s += multiplier.name;

                if (multiplier.power > 1)
                {
                    s += "^" + multiplier.power.ToString();
                }
            }

            s += term.dimension;

            return s;
        }

        public static string ToCode(Term[] equation, Dictionary<string, string> multiplier_map)
        {
            Dictionary<string, string> dimension_parts = new Dictionary<string, string>();

            foreach (Term term in equation)
            {
                string dimension_part;
                if (!dimension_parts.TryGetValue(term.dimension, out dimension_part))
                {
                    dimension_part = "";
                }

                if (dimension_part.Length > 0)
                {
                    dimension_part += " + ";
                }

                dimension_part += "(";

                string inside_parentheses = "";
                if (term.count != 1)
                {
                    inside_parentheses += term.count.ToString() + ".0f";
                }
                foreach (Multiplier multiplier in term.multipliers)
                {
                    string multiplier_in_code = multiplier_map[multiplier.name];

                    if (inside_parentheses.Length > 0)
                    {
                        inside_parentheses += " * ";
                    }
                    inside_parentheses += multiplier_in_code;

                    for (int i = 1; i < multiplier.power; ++i)
                    {
                        inside_parentheses += " * " + multiplier_in_code;
                    }
                }

                dimension_part += inside_parentheses + ")";

                dimension_parts[term.dimension] = dimension_part;
            }

            string s = "";
            foreach (KeyValuePair<string, string> dimension_part in dimension_parts)
            {
                s += dimension_part.Key + " = " + dimension_part.Value + "\n";
            }

            return s;
        }
    }

    class Program
    {
        static void Main(string[] args)
        {
            Term[] quat = { Term.Parse("s"), Term.Parse("izy"), Term.Parse("jxz"), Term.Parse("kyx") };
            Term[] vec = { Term.Parse("ax"), Term.Parse("by"), Term.Parse("cz") };
            Term[] inv_quat = { Term.Parse("s"), Term.Parse("-izy"), Term.Parse("-jxz"), Term.Parse("-kyx") };

            Term[] intermediate = Term.Multiply(quat, vec);
            Term[] result = Term.Multiply(intermediate, inv_quat);
            Console.WriteLine(Term.ToCode(result, new Dictionary<string, string> {
                ["s"] = "q.scalar",
                ["i"] = "q.zy",
                ["j"] = "q.xz",
                ["k"] = "q.yx",
                ["a"] = "v.x",
                ["b"] = "v.y",
                ["c"] = "v.z"
            }));
            Console.WriteLine();
            Console.WriteLine();



            Term[] quat_a = { Term.Parse("a"), Term.Parse("bzy"), Term.Parse("cxz"), Term.Parse("dyx") };
            Term[] quat_b = { Term.Parse("e"), Term.Parse("fzy"), Term.Parse("gxz"), Term.Parse("hyx") };
            result = Term.Multiply(quat_a, quat_b);
            Console.WriteLine(Term.ToCode(result, new Dictionary<string, string> {
                ["a"] = "a.scalar",
                ["b"] = "a.zy",
                ["c"] = "a.xz",
                ["d"] = "a.yx",
                ["e"] = "b.scalar",
                ["f"] = "b.zy",
                ["g"] = "b.xz",
                ["h"] = "b.yx"
            }));

            Console.WriteLine("Done");
        }
    }
}
