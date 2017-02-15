package my.fin.scanner;

import android.graphics.Bitmap;

/**
 * Created by yangyao on 15/02/2017.
 */

public class OpenCVHelper {
    static {
        System.loadLibrary("OpenCV");
    }
    public static native Bitmap scan(Bitmap bitmap);
}
