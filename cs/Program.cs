
using System.Numerics;
using System.Drawing;
using System.IO;

namespace zerhacken
{
    class Program
    {
        static void Main(string[] args)
        {
            int width = 512;
            int height = 512;

            Bitmap bitmap = new Bitmap(width, height, System.Drawing.Imaging.PixelFormat.Format32bppArgb);

            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    float u = (float)(x) / (float)(width);
                    float v = (float)(y) / (float)(height);
                   
                    bool conditional  = (x % 2) == 0 && (y % 2) == 0;
                    Color color = conditional ? Color.Red : Color.White;
                    bitmap.SetPixel(x, y, color);
                }
            }

            // Store bitmap as a png image on file system.
            using (MemoryStream ms = new MemoryStream())
            {
                bitmap.Save(ms, System.Drawing.Imaging.ImageFormat.Png);
                using (FileStream file = new FileStream("zerhacken.png", FileMode.Create, FileAccess.Write))
                {
                    ms.WriteTo(file);
                }
            }
        }
    }
}
