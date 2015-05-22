using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;
using OpenTK.Graphics;
using OpenTK.Graphics.OpenGL;

namespace OSD
{
    class oglUtility
    {
        public int Width;
        public int Height;

        class character
        {
            public Bitmap bitmap;
            public int gltextureid;
            public int width;
            public int size;
        }
        Dictionary<int, character> charDict = new Dictionary<int, character>();

        const float rad2deg = (float)(180 / Math.PI);
        const float deg2rad = (float)(1.0 / rad2deg);

        public void FillRectangle(Brush brushh, RectangleF rectf)
        {
            float x1 = rectf.X;
            float y1 = rectf.Y;

            float width = rectf.Width;
            float height = rectf.Height;

            GL.Begin(PrimitiveType.Quads);

            GL.LineWidth(0);

            if (((Type)brushh.GetType()) == typeof(LinearGradientBrush))
            {
                LinearGradientBrush temp = (LinearGradientBrush)brushh;
                GL.Color4(temp.LinearColors[0]);
            }
            else
            {
                GL.Color4(((SolidBrush)brushh).Color.R / 255f, ((SolidBrush)brushh).Color.G / 255f, ((SolidBrush)brushh).Color.B / 255f, ((SolidBrush)brushh).Color.A / 255f);
            }

            GL.Vertex2(x1, y1);
            GL.Vertex2(x1 + width, y1);

            if (((Type)brushh.GetType()) == typeof(LinearGradientBrush))
            {
                LinearGradientBrush temp = (LinearGradientBrush)brushh;
                GL.Color4(temp.LinearColors[1]);
            }
            else
            {
                GL.Color4(((SolidBrush)brushh).Color.R / 255f, ((SolidBrush)brushh).Color.G / 255f, ((SolidBrush)brushh).Color.B / 255f, ((SolidBrush)brushh).Color.A / 255f);
            }

            GL.Vertex2(x1 + width, y1 + height);
            GL.Vertex2(x1, y1 + height);
            GL.End();

        }

        public void DrawArc(Pen penn, RectangleF rect, float start, float degrees)
        {
            GL.LineWidth(penn.Width);
            GL.Color4(penn.Color);

            GL.Begin(PrimitiveType.LineStrip);

            start = 360 - start;
            start -= 30;

            float x = 0, y = 0;
            for (float i = start; i <= start + degrees; i++)
            {
                x = (float)Math.Sin(i * deg2rad) * rect.Width / 2;
                y = (float)Math.Cos(i * deg2rad) * rect.Height / 2;
                x = x + rect.X + rect.Width / 2;
                y = y + rect.Y + rect.Height / 2;
                GL.Vertex2(x, y);
            }
            GL.End();
        }

        public void DrawPolygon(Pen penn, PointF[] list)
        {
            GL.LineWidth(penn.Width);
            GL.Color4(penn.Color);

            GL.Begin(PrimitiveType.LineLoop);
            foreach (PointF pnt in list)
            {
                GL.Vertex2(pnt.X, pnt.Y);
            }

            GL.End();
        }

        public void DrawEllipse(Pen penn, Rectangle rect)
        {
            GL.LineWidth(penn.Width);
            GL.Color4(penn.Color);

            GL.Begin(PrimitiveType.LineLoop);
            float x, y;
            for (float i = 0; i < 360; i += 1)
            {
                x = (float)Math.Sin(i * deg2rad) * rect.Width / 2;
                y = (float)Math.Cos(i * deg2rad) * rect.Height / 2;
                x = x + rect.X + rect.Width / 2;
                y = y + rect.Y + rect.Height / 2;
                GL.Vertex2(x, y);
            }
            GL.End();
        }

        public void DrawLine(Pen penn, double x1, double y1, double x2, double y2)
        {
            GL.Color4(penn.Color);
            GL.LineWidth(penn.Width);

            GL.Begin(PrimitiveType.Lines);
            GL.Vertex2(x1, y1);
            GL.Vertex2(x2, y2);
            GL.End();
        }

        /*
         * @align: 0:left   1:right
         */
        public void DrawVScale(Pen pen, int align, float posx, float posy, int range, float mintick_step, float majtick_step)
        {
            //Because we just do simulation, we don't care the range, step and etc.
            float strOffset = 0f;
            mintick_step = 13f;
            float mintick_len = 5;
            float majtick_len = 10;
            int lineoffset = -1;
            Pen whitePen = new Pen(Color.White, 1);
            SolidBrush whiteBrush = new SolidBrush(whitePen.Color);
            Font font = new Font("Arial", 10);
            strOffset = calstring("0", font, 5, whiteBrush, 2);
            if (align == 1)
            {
                lineoffset = 1;
                strOffset = calstring("0", font, 5, whiteBrush, 0);
            }
            DrawLine(pen, posx, posy - mintick_step * 5, posx - (lineoffset * majtick_len), posy - mintick_step * 5);
            DrawLine(pen, posx, posy - mintick_step * 4, posx - (lineoffset * mintick_len), posy - mintick_step * 4);
            DrawLine(pen, posx, posy - mintick_step * 3, posx - (lineoffset * majtick_len), posy - mintick_step * 3);
            DrawLine(pen, posx, posy - mintick_step * 2, posx - (lineoffset * mintick_len), posy - mintick_step * 2);
            DrawLine(pen, posx, posy - mintick_step * 1, posx - (lineoffset * majtick_len), posy - mintick_step * 1);
            DrawLine(pen, posx, posy, posx - (lineoffset * mintick_len), posy);
            DrawLine(pen, posx, posy + mintick_step * 5, posx - (lineoffset * majtick_len), posy + mintick_step * 5);
            DrawLine(pen, posx, posy + mintick_step * 4, posx - (lineoffset * mintick_len), posy + mintick_step * 4);
            DrawLine(pen, posx, posy + mintick_step * 3, posx - (lineoffset * majtick_len), posy + mintick_step * 3);
            DrawLine(pen, posx, posy + mintick_step * 2, posx - (lineoffset * mintick_len), posy + mintick_step * 2);
            DrawLine(pen, posx, posy + mintick_step * 1, posx - (lineoffset * majtick_len), posy + mintick_step * 1);

            //draw number.
            PointF[] plist = new PointF[5];
            plist[0] = new PointF(posx - (lineoffset * (mintick_len + 1)), posy);
            plist[1] = new PointF(posx - (lineoffset * (mintick_len + 7)), posy - 6);
            plist[2] = new PointF(posx - (lineoffset * (mintick_len + 20)), posy - 6);
            plist[3] = new PointF(posx - (lineoffset * (mintick_len + 20)), posy + 6);
            plist[4] = new PointF(posx - (lineoffset * (mintick_len + 7)), posy + 6);

            DrawPolygon(pen, plist);
            drawstring("0", font, 5, whiteBrush, plist[2].X - strOffset, plist[2].Y);
        }

        Pen P = new Pen(Color.FromArgb(0x26, 0x27, 0x28), 2f);
        GraphicsPath pth = new GraphicsPath();
        public void drawstring(string text, Font font, float fontsize, SolidBrush brush, float x, float y)
        {
            if (text == null || text == "")
                return;

            char[] chars = text.ToCharArray();

            float maxy = 1;

            foreach (char cha in chars)
            {
                int charno = (int)cha;

                int charid = charno ^ (int)(fontsize * 1000) ^ brush.Color.ToArgb();

                if (!charDict.ContainsKey(charid))
                {
                    charDict[charid] = new character() { bitmap = new Bitmap(128, 128, System.Drawing.Imaging.PixelFormat.Format32bppArgb), size = (int)fontsize };

                    charDict[charid].bitmap.MakeTransparent(Color.Transparent);

                    //charbitmaptexid

                    float maxx = this.Width / 150; // for space


                    // create bitmap
                    using (Graphics gfx = Graphics.FromImage(charDict[charid].bitmap))
                    {
                        pth.Reset();

                        if (text != null)
                            pth.AddString(cha + "", font.FontFamily, 0, fontsize + 5, new Point((int)0, (int)0), StringFormat.GenericTypographic);

                        gfx.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.AntiAlias;

                        gfx.DrawPath(P, pth);

                        //Draw the face

                        gfx.FillPath(brush, pth);


                        if (pth.PointCount > 0)
                        {
                            foreach (PointF pnt in pth.PathPoints)
                            {
                                if (pnt.X > maxx)
                                    maxx = pnt.X;

                                if (pnt.Y > maxy)
                                    maxy = pnt.Y;
                            }
                        }
                    }

                    charDict[charid].width = (int)(maxx + 2);

                    //charbitmaps[charid] = charbitmaps[charid].Clone(new RectangleF(0, 0, maxx + 2, maxy + 2), charbitmaps[charid].PixelFormat);

                    //charbitmaps[charno * (int)fontsize].Save(charno + " " + (int)fontsize + ".png");

                    // create texture
                    int textureId;
                    GL.TexEnv(TextureEnvTarget.TextureEnv, TextureEnvParameter.TextureEnvMode, (float)TextureEnvModeCombine.Replace);//Important, or wrong color on some computers

                    Bitmap bitmap = charDict[charid].bitmap;
                    GL.GenTextures(1, out textureId);
                    GL.BindTexture(TextureTarget.Texture2D, textureId);

                    BitmapData data = bitmap.LockBits(new System.Drawing.Rectangle(0, 0, bitmap.Width, bitmap.Height), ImageLockMode.ReadOnly, System.Drawing.Imaging.PixelFormat.Format32bppArgb);

                    GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, data.Width, data.Height, 0, OpenTK.Graphics.OpenGL.PixelFormat.Bgra, PixelType.UnsignedByte, data.Scan0);

                    GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Linear);
                    GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Linear);

                    //    GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)All.Nearest);
                    //GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)All.Nearest);
                    GL.Finish();
                    bitmap.UnlockBits(data);

                    charDict[charid].gltextureid = textureId;
                }

                //GL.Enable(EnableCap.Blend);
                GL.BlendFunc(BlendingFactorSrc.SrcAlpha, BlendingFactorDest.OneMinusSrcAlpha);

                GL.Enable(EnableCap.Texture2D);
                GL.BindTexture(TextureTarget.Texture2D, charDict[charid].gltextureid);

                float scale = 1.0f;

                GL.Begin(PrimitiveType.Quads);
                GL.TexCoord2(0, 0); GL.Vertex2(x, y);
                GL.TexCoord2(1, 0); GL.Vertex2(x + charDict[charid].bitmap.Width * scale, y);
                GL.TexCoord2(1, 1); GL.Vertex2(x + charDict[charid].bitmap.Width * scale, y + charDict[charid].bitmap.Height * scale);
                GL.TexCoord2(0, 1); GL.Vertex2(x + 0, y + charDict[charid].bitmap.Height * scale);
                GL.End();

                //GL.Disable(EnableCap.Blend);
                GL.Disable(EnableCap.Texture2D);

                x += charDict[charid].width * scale;
            }
        }

        public float calstring(string text, Font font, float fontsize, SolidBrush brush, int align)
        {
            if (text == null || text == "" || align == 0)
                return 0;

            char[] chars = text.ToCharArray();

            float maxy = 1;
            float strLen = 0;
            foreach (char cha in chars)
            {
                int charno = (int)cha;

                int charid = charno ^ (int)(fontsize * 1000) ^ brush.Color.ToArgb();

                if (!charDict.ContainsKey(charid))
                {
                    charDict[charid] = new character() { bitmap = new Bitmap(128, 128, System.Drawing.Imaging.PixelFormat.Format32bppArgb), size = (int)fontsize };

                    charDict[charid].bitmap.MakeTransparent(Color.Transparent);

                    //charbitmaptexid

                    float maxx = this.Width / 150; // for space


                    // create bitmap
                    using (Graphics gfx = Graphics.FromImage(charDict[charid].bitmap))
                    {
                        pth.Reset();

                        if (text != null)
                            pth.AddString(cha + "", font.FontFamily, 0, fontsize + 5, new Point((int)0, (int)0), StringFormat.GenericTypographic);

                        gfx.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.AntiAlias;

                        gfx.DrawPath(P, pth);

                        //Draw the face

                        gfx.FillPath(brush, pth);


                        if (pth.PointCount > 0)
                        {
                            foreach (PointF pnt in pth.PathPoints)
                            {
                                if (pnt.X > maxx)
                                    maxx = pnt.X;

                                if (pnt.Y > maxy)
                                    maxy = pnt.Y;
                            }
                        }
                    }

                    charDict[charid].width = (int)(maxx + 2);

                    //charbitmaps[charid] = charbitmaps[charid].Clone(new RectangleF(0, 0, maxx + 2, maxy + 2), charbitmaps[charid].PixelFormat);

                    //charbitmaps[charno * (int)fontsize].Save(charno + " " + (int)fontsize + ".png");

                    // create texture
                    int textureId;
                    GL.TexEnv(TextureEnvTarget.TextureEnv, TextureEnvParameter.TextureEnvMode, (float)TextureEnvModeCombine.Replace);//Important, or wrong color on some computers

                    Bitmap bitmap = charDict[charid].bitmap;
                    GL.GenTextures(1, out textureId);
                    GL.BindTexture(TextureTarget.Texture2D, textureId);

                    BitmapData data = bitmap.LockBits(new System.Drawing.Rectangle(0, 0, bitmap.Width, bitmap.Height), ImageLockMode.ReadOnly, System.Drawing.Imaging.PixelFormat.Format32bppArgb);

                    GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, data.Width, data.Height, 0, OpenTK.Graphics.OpenGL.PixelFormat.Bgra, PixelType.UnsignedByte, data.Scan0);

                    GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Linear);
                    GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Linear);
                    GL.Finish();
                    bitmap.UnlockBits(data);

                    charDict[charid].gltextureid = textureId;
                }

                float scale = 1.0f;

                strLen += charDict[charid].width * scale;
            }

            if (align == 1)
            {
                strLen = strLen / 2;
            }
            return strLen;
        }
    }
}
