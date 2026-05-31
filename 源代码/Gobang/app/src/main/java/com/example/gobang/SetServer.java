package com.example.gobang;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

public class SetServer extends AppCompatActivity {

    public static final String FILENAME = "data";
    public static final String IP = "ip";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_set_server);
        getSupportActionBar().setDisplayOptions(ActionBar.DISPLAY_SHOW_HOME | ActionBar.DISPLAY_SHOW_TITLE);
        getSupportActionBar().setIcon(R.mipmap.icon);
        getSupportActionBar().setTitle("五子棋智能评分系统");
        EditText ip = findViewById(R.id.username);
        Button confirm = findViewById(R.id.login);
        confirm.setOnClickListener(v -> {
            String address = ip.getText().toString();
            //更改服务器的IP地址，并将其保存在SharedPreferences中
            if (checkIP(address)) {
                SharedPreferences preferences = this.getSharedPreferences(FILENAME, Context.MODE_PRIVATE);
                SharedPreferences.Editor editor = preferences.edit();
                editor.putString(IP, address);
                editor.commit();
            }
            else {
                Toast.makeText(this, "IP地址格式不正确", Toast.LENGTH_SHORT).show();
            }
            finish();
        });
    }

    public static String getIP(Context context) {
        SharedPreferences preferences = context.getSharedPreferences(FILENAME, Context.MODE_PRIVATE);
        return preferences.getString(IP, "124.71.192.189"); //124.71.192.189, 10.0.2.2
    }

    public static boolean checkIP(String ip) {
        String[] parts = ip.split("\\.");
        if (parts.length != 4)
            return false;
        try {
            for (int i = 0; i < 4; ++i) {
                int val = Integer.parseInt(parts[i]);
                if (val < 0 || val > 255)
                    return false;
            }
        }
        catch (NumberFormatException e) {
            return false;
        }
        return true;
    }
}