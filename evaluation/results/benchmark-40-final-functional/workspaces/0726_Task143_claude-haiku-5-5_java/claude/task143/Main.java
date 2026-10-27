package claude.task143;
public class Main {

    public static void sort(int[] arr) {
        if (arr == null || arr.length < 2) {
            return;
        }
        mergeSort(arr, 0, arr.length - 1, new int[arr.length]);
    }

    private static void mergeSort(int[] arr, int left, int right, int[] temp) {
        if (left >= right) {
            return;
        }
        int mid = left + (right - left) / 2;
        mergeSort(arr, left, mid, temp);
        mergeSort(arr, mid + 1, right, temp);
        merge(arr, left, mid, right, temp);
    }

    private static void merge(int[] arr, int left, int mid, int right, int[] temp) {
        int i = left, j = mid + 1, k = left;
        while (i <= mid && j <= right) {
            if (arr[i] <= arr[j]) {
                temp[k++] = arr[i++];
            } else {
                temp[k++] = arr[j++];
            }
        }
        while (i <= mid) {
            temp[k++] = arr[i++];
        }
        while (j <= right) {
            temp[k++] = arr[j++];
        }
        for (int x = left; x <= right; x++) {
            arr[x] = temp[x];
        }
    }

    public static int[] sortArray(int[] input) {
        if (input == null) {
            return new int[0];
        }
        int[] result = input.clone();
        sort(result);
        return result;
    }

    public static void main(String[] args) {
        int[] data;
        if (args.length > 0) {
            data = new int[args.length];
            for (int i = 0; i < args.length; i++) {
                data[i] = Integer.parseInt(args[i].trim());
            }
        } else {
            java.util.Scanner scanner = new java.util.Scanner(System.in);
            java.util.List<Integer> list = new java.util.ArrayList<>();
            while (scanner.hasNextInt()) {
                list.add(scanner.nextInt());
            }
            scanner.close();
            data = new int[list.size()];
            for (int i = 0; i < list.size(); i++) {
                data[i] = list.get(i);
            }
        }

        int[] sorted = sortArray(data);

        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < sorted.length; i++) {
            if (i > 0) {
                sb.append(' ');
            }
            sb.append(sorted[i]);
        }
        System.out.println(sb.toString());
    }
}