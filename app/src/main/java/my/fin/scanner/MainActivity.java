package my.fin.scanner;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;

public class MainActivity extends AppCompatActivity {

    Bitmap origBitmap;
    Bitmap outputBitmap;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initView();


    }

    private void initView() {
        final ImageView img = (ImageView) findViewById(R.id.imageView);
        Button scanBtn= (Button) findViewById(R.id.button);

        BitmapFactory.Options o=new BitmapFactory.Options();

        // TODO: 29/08/2016  May need to check sample size https://developer.android.com/training/displaying-bitmaps/load-bitmap.html
        o.inSampleSize = 4;
        o.inDither=false;
        origBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.card4, o);
        img.setImageBitmap(origBitmap);
        scanBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                scan(img);
            }
        });

    }

    private void scan(ImageView img) {
        outputBitmap = my.fin.scanner.OpenCVHelper.scan(origBitmap);
        img.setImageBitmap(outputBitmap);
    }
}
