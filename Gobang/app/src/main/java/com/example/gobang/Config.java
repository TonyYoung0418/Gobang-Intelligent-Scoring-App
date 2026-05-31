package com.example.gobang;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

//“设置”界面
public class Config extends AppCompatActivity {

    public static final String FILENAME = "config";
    public static final String FIRST = "first";
    public static final String DIFF = "diff";
    private RadioButton[] radioButtons = new RadioButton[3];
    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR1)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_config);
        getSupportActionBar().setDisplayOptions(ActionBar.DISPLAY_SHOW_HOME | ActionBar.DISPLAY_SHOW_TITLE);
        getSupportActionBar().setIcon(R.mipmap.icon);
        getSupportActionBar().setTitle("五子棋智能评分系统");
        TextView view1 = findViewById(R.id.textView2);
        view1.setTextSize(20f);


        TextView view2 = findViewById(R.id.textView3);
        view2.setTextSize(20f);
        view2.setTextAlignment(View.TEXT_ALIGNMENT_GRAVITY);

        Button button = findViewById(R.id.button);
        button.setOnClickListener(v -> {
            finish();
        });

        Switch switch_first = findViewById(R.id.switch_first);
        switch_first.setChecked(isFirst(this));
        switch_first.setOnCheckedChangeListener((buttonView, isChecked) -> {
            //将用户偏好通过SharedPreferences保存
            SharedPreferences preferences = this.getSharedPreferences(FILENAME, Context.MODE_PRIVATE);
            SharedPreferences.Editor editor = preferences.edit();
            editor.putBoolean(FIRST, isChecked);
            editor.commit();
            Toast.makeText(this, "重新开始游戏后将改变走棋顺序", Toast.LENGTH_SHORT).show();
        });

        radioButtons[0] = findViewById(R.id.low);
        radioButtons[1] = findViewById(R.id.mid);
        radioButtons[2] = findViewById(R.id.high);
        radioButtons[getDiff(this)].setChecked(true);

        RadioGroup group = findViewById(R.id.group);
        group.setOnCheckedChangeListener((x, y) -> {
            for (int i = 0; i < 3; ++i) {
                if (radioButtons[i].isChecked()) {
                    SharedPreferences preferences = this.getSharedPreferences(FILENAME, Context.MODE_PRIVATE);
                    SharedPreferences.Editor editor = preferences.edit();
                    editor.putInt(DIFF, i);
                    editor.commit();
                    break;
                }
            }
        });
    }

    public static boolean isFirst(Context context) {
        SharedPreferences preferences = context.getSharedPreferences(FILENAME, Context.MODE_PRIVATE);
        return preferences.getBoolean(FIRST, false);
    }

    public static int getDiff(Context context) {
        SharedPreferences preferences = context.getSharedPreferences(FILENAME, Context.MODE_PRIVATE);
        return preferences.getInt(DIFF, 2);
    }
}