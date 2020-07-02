package com.library.imgcvt;

import android.graphics.Bitmap;
import android.graphics.Rect;
import android.os.Bundle;

import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.android.material.snackbar.Snackbar;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.constraintlayout.solver.widgets.Rectangle;

import android.os.Environment;
import android.util.AttributeSet;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TextView;
import org.w3c.dom.Text;
import java.lang.reflect.Array;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;


public class MainActivity extends AppCompatActivity {
    int[] renderResult;
    float mZoom = 1;
    int width = 900;
    int height = 900;

    private class DrawView extends View {
        Paint paint = new Paint();

        private void init() {
            paint.setColor(Color.BLACK);
        }

        public DrawView(Context context) {
            super(context);
            init();
            renderResult = null;
        }

        public DrawView(Context context, AttributeSet attrs) {
            super(context, attrs);
            init();
        }

        public DrawView(Context context, AttributeSet attrs, int defStyle) {
            super(context, attrs, defStyle);
            init();
        }

        @Override
        public void onDraw(Canvas canvas) {
            if(renderResult == null) return;
            Bitmap bit = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
            bit.setPixels(renderResult, 0, width, 0, 0, width, height);
            canvas.drawBitmap(bit, null, new Rect(0, 0, width, height), null);
        }
    }

    DrawView drawView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if(!IsDllFound())
            return;
        if(InitLibrary() != 0)
            return;
        String path = Environment.getExternalStorageDirectory().getAbsolutePath();
        path = path.concat("/Download/5.fbx");
        String ret = LoadAsset(path, false, 5);
        if(ret != null) {
            TextView tv = new TextView(this);
            tv.setText(ret);
            setContentView(tv);
            return;
        }

        drawView = new DrawView(this);
        drawView.setBackgroundColor(Color.WHITE);
        drawView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                renderResult = null;
                SetResolution(width, height);
                mZoom += 0.1f;
                SetROI(0.5f);       // Metallic value 0~5
                SetZoomFactor(mZoom);
                SetRotation(0.3f, 0.3f, 0.3f);
                renderResult = GetChromeImage(255);
                //GetLargeBinary();
                //GetSmallBinary();
                drawView.invalidate();
            }
        });
        setContentView(drawView);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    public native boolean IsDllFound();
    public native int InitLibrary();
    public native String LoadAsset(String path, boolean optMesh, double optRate);
    public native boolean SetResolution(int w, int h);
    public native int SetROI(double roi);
    public native boolean SetRotation(float rX, float rY, float rZ);
    public native boolean SetZoomFactor(double zf);  //zf: 1.0 ~ 2.0
    public native int[] GetMeshImage(int background);
    public native int[] GetSurface(int background);
    public native int[] GetChromeImage(int background);
    public native int[] GetNeonImage(int background);
    public native int[] GetGridImage(int nGridNum, float nGridThickness, int background); //nGridNum: 10~30, nGridThickness: 0.2~0.8
    public native int[] GetNormalImage(int background);
    public native byte[] GetBinary();
    static {
        System.loadLibrary("libImgCvt");
    }
}
